#include "platform.h"

#include "musashi_boot_memory.h"
#include "musashi_native_lane.h"
#include "musashi_x11_nograb.h"
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

const char *musashi_platform_name(void) {
    return "windows";
}

char *musashi_platform_launch_log_path(void) {
    DWORD capacity = GetEnvironmentVariableW(L"BFM_LAUNCH_LOG", NULL, 0);
    if (!capacity) return NULL;
    WCHAR *wide = malloc((size_t)capacity * sizeof(*wide));
    if (!wide) return NULL;
    DWORD length = GetEnvironmentVariableW(L"BFM_LAUNCH_LOG", wide, capacity);
    char *path = NULL;
    if (length && length < capacity) {
        int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1,
                                       NULL, 0, NULL, NULL);
        if (size > 0) {
            path = malloc((size_t)size);
            if (path && !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1,
                                             path, size, NULL, NULL)) {
                free(path);
                path = NULL;
            }
        }
    }
    free(wide);
    return path;
}

/* Views of a section must start on the 64 KiB allocation granularity; both
 * guest bases (0x80000000, 0x1F800000) and the scratchpad's offset
 * (MUSASHI_RAM_SIZE) are multiples of it. */
#define GUEST_GRANULE 0x10000u
#define SCRATCHPAD_BASE 0x1f800000u

void *musashi_native_lane_map_guest(size_t size, int *scratchpad_aliased) {
    /* One pagefile-backed section backs the guest window at 0x80000000; its
     * granule at offset MUSASHI_RAM_SIZE (MusashiBootMemory.scratchpad) is
     * mapped a second time at 0x1F800000, so natively compiled code that
     * dereferences a scratchpad pointer sees the interpreter's scratchpad.
     * As on Linux, a base that is already taken refuses the window. */
    size_t total = (size + GUEST_GRANULE - 1u) & ~(size_t)(GUEST_GRANULE - 1u);
    HANDLE section;
    void *window, *pad;
    DWORD old;
    if (scratchpad_aliased) *scratchpad_aliased = 0;
    section = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                 (DWORD)((uint64_t)total >> 32), (DWORD)total, NULL);
    if (!section) return NULL;
    window = MapViewOfFileEx(section, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, total,
                             (void *)(uintptr_t)MUSASHI_NATIVE_LANE_GUEST_BASE);
    if (!window || (uintptr_t)window != MUSASHI_NATIVE_LANE_GUEST_BASE) {
        if (window) UnmapViewOfFile(window);
        CloseHandle(section);
        return NULL;
    }
    if ((MUSASHI_RAM_SIZE % GUEST_GRANULE) == 0u && total >= MUSASHI_RAM_SIZE + GUEST_GRANULE) {
        pad = MapViewOfFileEx(section, FILE_MAP_READ | FILE_MAP_WRITE, 0, MUSASHI_RAM_SIZE,
                              GUEST_GRANULE, (void *)(uintptr_t)SCRATCHPAD_BASE);
        if (pad && (uintptr_t)pad == SCRATCHPAD_BASE) {
            /* The Linux alias is one 4 KiB page; keep the rest of this
             * granule (the mirror guard's first pages) unreachable, so
             * 1F801000+ faults on both hosts. */
            if (VirtualProtect((uint8_t *)pad + 0x1000u, GUEST_GRANULE - 0x1000u, PAGE_NOACCESS, &old)) {
                if (scratchpad_aliased) *scratchpad_aliased = 1;
            } else {
                UnmapViewOfFile(pad);
            }
        } else if (pad) {
            UnmapViewOfFile(pad);
        }
    }
    /* The views keep the section alive. */
    CloseHandle(section);
    return window;
}

int musashi_host_protect_none(void *addr, size_t size) {
    DWORD old;
    return VirtualProtect(addr, size, PAGE_NOACCESS, &old) ? 0 : -1;
}

/* Format without the CRT, write, and end the process with the status a
 * SIGSEGV gives on Linux, so gate scripts read the same rc. */
static LONG report_fault(const EXCEPTION_RECORD *record, int claim_all) {
    char buf[128];
    static const char hex[] = "0123456789abcdef";
    uint64_t addr;
    uint32_t pc;
    int i, n = 0;
    const char *head = "native_boot: HOST_FAULT addr=";
    DWORD written;
    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION &&
        record->ExceptionCode != EXCEPTION_IN_PAGE_ERROR)
        return EXCEPTION_CONTINUE_SEARCH;
    addr = record->NumberParameters >= 2 ? (uint64_t)record->ExceptionInformation[1] : 0u;
    pc = musashi_native_lane_current();
    /* First chance, ahead of any frame's SEH: only a fault inside a lane
     * call or on the guest window is ours. Other code (a GL driver probing
     * memory under __try) keeps its own handling; a fault nobody handles
     * reaches the unhandled filter and is reported the same way. */
    if (!claim_all && !pc &&
        !(addr >= MUSASHI_NATIVE_LANE_GUEST_BASE && addr < MUSASHI_NATIVE_LANE_GUEST_BASE + 0x800000u) &&
        !(addr >= SCRATCHPAD_BASE && addr < SCRATCHPAD_BASE + GUEST_GRANULE))
        return EXCEPTION_CONTINUE_SEARCH;
    while (*head) buf[n++] = *head++;
    for (i = 60; i >= 0; i -= 4) buf[n++] = hex[(addr >> i) & 15u];
    head = " lane_pc=";
    while (*head) buf[n++] = *head++;
    for (i = 28; i >= 0; i -= 4) buf[n++] = hex[(pc >> i) & 15u];
    buf[n++] = '\n';
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), buf, (DWORD)n, &written, NULL);
    TerminateProcess(GetCurrentProcess(), 139);
    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG CALLBACK lane_fault(EXCEPTION_POINTERS *info) {
    return report_fault(info->ExceptionRecord, 0);
}

static LONG WINAPI unhandled_fault(EXCEPTION_POINTERS *info) {
    return report_fault(info->ExceptionRecord, 1);
}

void musashi_host_install_fault_report(void) {
    AddVectoredExceptionHandler(1, lane_fault);
    SetUnhandledExceptionFilter(unhandled_fault);
}

void *musashi_host_symbol(const char *name) {
    /* The optional entry points are SDL's; the port links SDL2.dll. */
    HMODULE sdl = GetModuleHandleW(L"SDL2.dll");
    return sdl ? (void *)GetProcAddress(sdl, name) : NULL;
}

/* No X server on this host; native_boot calls these only for an x11 window. */
void musashi_x11_spectator(void *display, unsigned long window) {
    (void)display;
    (void)window;
}

void musashi_x11_spectator_focused(void) {
}

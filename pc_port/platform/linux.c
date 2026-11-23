/* syscall(SYS_memfd_create) and ftruncate for the guest window are POSIX/GNU,
 * hidden under a strict -std=c99 build (the probe tests compile this file so). */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "platform.h"

#include "musashi_boot_memory.h"
#include "musashi_native_lane.h"
#include <dlfcn.h>
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <spawn.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

const char *musashi_platform_name(void) {
    return "linux";
}

char *musashi_platform_launch_log_path(void) {
    const char *path = getenv("BFM_LAUNCH_LOG");
    return path && *path ? strdup(path) : NULL;
}

static int locale_uses_utf8(const char *name) {
    locale_t locale = newlocale(LC_CTYPE_MASK, name, (locale_t)0);
    if (!locale) return 0;
    const char *codeset = nl_langinfo_l(CODESET, locale);
    int utf8 = strcmp(codeset, "UTF-8") == 0 || strcmp(codeset, "UTF8") == 0;
    freelocale(locale);
    return utf8;
}

int musashi_platform_set_dialog_locale(void) {
    /* Only the dedicated dialog process changes its environment. SDL's X11
     * backend reads it again when it creates the font set. */
    const char *choices[] = {"C.UTF-8", "en_US.UTF-8", getenv("LANG")};
    for (size_t i = 0; i < sizeof(choices) / sizeof(choices[0]); ++i) {
        if (choices[i] && *choices[i] && locale_uses_utf8(choices[i]))
            return setenv("LC_ALL", choices[i], 1) == 0;
    }
    return 0;
}

int musashi_platform_show_utf8_dialog(const char *message) {
    const unsigned char *p = (const unsigned char *)message;
    while (*p && *p < 0x80) ++p;
    if (!*p || locale_uses_utf8("")) return 0;

    /* The running game can have audio and rendering threads. Start a fresh
     * process so changing the locale cannot affect those threads. */
    char *const args[] = {"musashi_native_boot", "--show-error-dialog-utf8", (char *)message, NULL};
    pid_t child;
    if (posix_spawn(&child, "/proc/self/exe", NULL, NULL, args, environ) != 0) return 0;
    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    return waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void *musashi_native_lane_map_guest(size_t size, int *scratchpad_aliased) {
    /* One shared memfd backs the guest window at 0x80000000; its page at
     * offset MUSASHI_RAM_SIZE (MusashiBootMemory.scratchpad, page aligned) is
     * mapped a second time at 0x1F800000, so natively compiled code that
     * dereferences a scratchpad pointer sees the interpreter's scratchpad. */
    long page = sysconf(_SC_PAGESIZE);
    size_t total = (size + (size_t)page - 1u) & ~((size_t)page - 1u);
    int fd = (int)syscall(SYS_memfd_create, "musashi-guest", 0u);
    void *window, *pad;
    if (scratchpad_aliased) *scratchpad_aliased = 0;
    if (fd < 0) return NULL;
    if (ftruncate(fd, (off_t)total) != 0) { close(fd); return NULL; }
    window = mmap((void *)(uintptr_t)MUSASHI_NATIVE_LANE_GUEST_BASE, total, PROT_READ | PROT_WRITE,
                  MAP_SHARED | 0x100000 /* MAP_FIXED_NOREPLACE */, fd, 0);
    if (window == MAP_FAILED || (uintptr_t)window != MUSASHI_NATIVE_LANE_GUEST_BASE) {
        if (window != MAP_FAILED) munmap(window, total);
        close(fd);
        return NULL;
    }
    if ((MUSASHI_RAM_SIZE % (uint32_t)page) == 0u && total > MUSASHI_RAM_SIZE) {
        pad = mmap((void *)(uintptr_t)0x1f800000u, (size_t)page, PROT_READ | PROT_WRITE,
                   MAP_SHARED | 0x100000, fd, (off_t)MUSASHI_RAM_SIZE);
        if (pad != MAP_FAILED && (uintptr_t)pad == 0x1f800000u) {
            if (scratchpad_aliased) *scratchpad_aliased = 1;
        } else if (pad != MAP_FAILED) {
            munmap(pad, (size_t)page);
        }
    }
    close(fd);
    return window;
}

int musashi_host_protect_none(void *addr, size_t size) {
    return mprotect(addr, size, PROT_NONE);
}

/* Async-signal-safe formatting only. */
static void lane_fault(int sig, siginfo_t *info, void *context) {
    char buf[128];
    static const char hex[] = "0123456789abcdef";
    uint64_t addr = (uint64_t)(uintptr_t)info->si_addr;
    uint32_t pc = musashi_native_lane_current();
    int i, n = 0;
    const char *head = "native_boot: HOST_FAULT addr=";
    (void)context;
    while (*head) buf[n++] = *head++;
    for (i = 60; i >= 0; i -= 4) buf[n++] = hex[(addr >> i) & 15u];
    head = " lane_pc=";
    while (*head) buf[n++] = *head++;
    for (i = 28; i >= 0; i -= 4) buf[n++] = hex[(pc >> i) & 15u];
    buf[n++] = '\n';
    (void)!write(2, buf, (size_t)n);
    signal(sig, SIG_DFL);
    raise(sig);
}

void musashi_host_install_fault_report(void) {
    struct sigaction action;
    memset(&action, 0, sizeof action);
    action.sa_sigaction = lane_fault;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
}

void *musashi_host_symbol(const char *name) {
    return dlsym(RTLD_DEFAULT, name);
}

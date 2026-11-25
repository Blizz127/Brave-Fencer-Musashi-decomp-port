/* A cold run from the caller's US disc to named public ABI boundaries. */
#ifndef BFM_TEST_STARTUP_MOUNTED_FIXTURE_H
#define BFM_TEST_STARTUP_MOUNTED_FIXTURE_H

#include "musashi_boot_memory.h"
#include "musashi_native_lane.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    BFM_GAME_ENTRY = 0x80010178u,
    BFM_GRAPHICS_ENTRY = 0x800141f0u,
    BFM_AFTER_GRAPHICS = 0x800101c8u,
    BFM_CD_ENTRY = 0x8005fc68u
};

typedef struct {
    MusashiBootMemory *memory;
    uint32_t endpoint, interrupt_at;
    uint64_t clocks, entry_clock, graphics_clock, return_clock, cd_clock;
    MusashiEntryCpuSnapshot entry, graphics, returned, cd;
    MusashiEntryRunStop stop;
    unsigned reached, interrupt_pending, interrupted, change_word;
} StartupMountedFixture;

static int startup_mounted_clock(void *userdata, uint32_t cycles) {
    StartupMountedFixture *f = userdata;
    f->clocks += cycles;
    if (f->interrupt_pending) {
        f->interrupt_pending = 0;
        f->interrupted = 1;
        if (!f->change_word) return 0;
        uint8_t *word = musashi_boot_ram_span(f->memory, f->interrupt_at, 4u);
        if (!word) return 0;
        word[0] ^= 1u;
    }
    return 1;
}

static int startup_mounted_checkpoint(void *userdata, const void *continuation) {
    StartupMountedFixture *f = userdata;
    return continuation && !f->reached;
}

static void startup_mounted_observe(void *userdata, const MusashiEntryCpuSnapshot *cpu) {
    StartupMountedFixture *f = userdata;
    switch (cpu->pc) {
    case BFM_GAME_ENTRY: f->entry = *cpu; f->entry_clock = f->clocks; break;
    case BFM_GRAPHICS_ENTRY: f->graphics = *cpu; f->graphics_clock = f->clocks; break;
    case BFM_AFTER_GRAPHICS: f->returned = *cpu; f->return_clock = f->clocks; break;
    case BFM_CD_ENTRY: f->cd = *cpu; f->cd_clock = f->clocks; break;
    }
    if (cpu->pc == f->endpoint) f->reached = 1;
    if (cpu->pc == f->interrupt_at) f->interrupt_pending = 1;
}

static void startup_mounted_sink(void *userdata, const uint8_t *bytes, int32_t length) {
    (void)userdata;
    (void)bytes;
    (void)length;
}

/* A missing image skips. A present image must pass the normal full-file check. */
static int startup_mounted_load(const char *path, MusashiBootMemory **out) {
    FILE *file = fopen(path, "rb");
    if (!file) return 77;
    if (fseek(file, 0, SEEK_END)) { fclose(file); return 1; }
    long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET)) { fclose(file); return 1; }
    uint8_t *exe = malloc((size_t)length);
    if (!exe) { fclose(file); return 1; }
    int read = fread(exe, 1, (size_t)length, file) == (size_t)length;
    fclose(file);
    int aliased = 0;
    MusashiBootMemory *memory = musashi_native_lane_map_guest(sizeof *memory, &aliased);
    if (!memory) { free(exe); return 77; }
    int mapped = read && musashi_boot_map_exe(memory, exe, (size_t)length);
    free(exe);
    if (!mapped) return 1;
    *out = memory;
    return 0;
}

static MusashiResetGraphPrefixStatus startup_mounted_run(StartupMountedFixture *f) {
    MusashiCallbackDevice callback = {.userdata=f, .checkpoint=startup_mounted_checkpoint};
    MusashiExecutionClock clock = {.userdata=f, .advance=startup_mounted_clock};
    MusashiStartupPrefixDevice devices = {.callback=&callback, .clock=&clock};
    return musashi_boot_run_entry_with_devices(f->memory, 0, &devices,
        startup_mounted_sink, NULL, startup_mounted_observe, f, &f->stop);
}

#endif

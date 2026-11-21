#ifndef MUSASHI_TEST_RESET_QUERY_PROBE_H
#define MUSASHI_TEST_RESET_QUERY_PROBE_H

#include "musashi_boot_memory.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The checked CPU test runner returns 1 only after verifying the device
 * operations, guest return and refusal cases for the selected entry. */
int musashi_test_owned_reset(const unsigned char *exe, size_t size, int query_only);

typedef int (*ResetQueryCall)(MusashiBootMemory *, const MusashiGpuDevice *, int32_t, int32_t *);

static uint32_t count_read32(void *user, uint32_t address)
{
    (void)address;
    ++*(unsigned *)user;
    return 0;
}

static uint16_t count_read16(void *user, uint32_t address)
{
    (void)address;
    ++*(unsigned *)user;
    return 0;
}

static void count_write32(void *user, uint32_t address, uint32_t value)
{
    (void)address;
    (void)value;
    ++*(unsigned *)user;
}

static void count_write16(void *user, uint32_t address, uint16_t value)
{
    (void)address;
    (void)value;
    ++*(unsigned *)user;
}

static int direct_call_refuses(ResetQueryCall call, MusashiBootMemory *guest)
{
    static MusashiBootMemory before;
    unsigned callbacks = 0;
    MusashiGpuDevice device = {
        .read32 = count_read32, .write32 = count_write32, .userdata = &callbacks,
        .read16 = count_read16, .write16 = count_write16
    };
    const int32_t initial_result = -34567;
    int32_t result = initial_result;
    memcpy(&before, guest, sizeof before);
    if (call(guest, &device, 0, &result) != 0 || callbacks != 0 ||
        result != initial_result || memcmp(&before, guest, sizeof before) != 0) {
        fputs("Direct GPU binding changed state or accepted a call without a CPU frame.\n", stderr);
        return 0;
    }
    return 1;
}

static int run_reset_query_probe(int argc, char **argv, ResetQueryCall call, int query_only)
{
    /* The public boot mapper authenticates the entire supported executable. */
    static unsigned char exe[0x65000];
    static MusashiBootMemory guest;
    const char *override;
    FILE *input = NULL;

    memset(&guest, 0x59, sizeof guest);
    if (!direct_call_refuses(call, &guest))
        return 1;

    if (argc > 2) {
        fputs("Expected at most one executable path.\n", stderr);
        return 1;
    }
    if (argc == 2 && strcmp(argv[1], "--direct-only") == 0)
        return 0;
    override = getenv("MUSASHI_TEST_EXE");
    if (argc == 2 && argv[1][0]) {
        input = fopen(argv[1], "rb");
        if (!input && errno != ENOENT) {
            perror("Could not open the executable argument");
            return 1;
        }
    }
    if (!input && override && override[0]) {
        input = fopen(override, "rb");
        if (!input && errno != ENOENT) {
            perror("Could not open MUSASHI_TEST_EXE");
            return 1;
        }
    }
    if (!input) {
        puts("Direct refusal passed. Owned executable unavailable for the CPU route.");
        return 77;
    }

    size_t count = fread(exe, 1, sizeof exe, input);
    int next = fgetc(input);
    int read_failed = ferror(input);
    int close_failed = fclose(input);
    memset(&guest, 0, sizeof guest);
    if (count != sizeof exe || next != EOF || read_failed || close_failed ||
        !musashi_boot_map_exe(&guest, exe, sizeof exe)) {
        fputs("Owned executable failed the complete USA image identity check.\n", stderr);
        return 1;
    }
    if (!direct_call_refuses(call, &guest))
        return 1;
    if (!musashi_test_owned_reset(exe, sizeof exe, query_only)) {
        fputs("Checked GPU CPU route failed.\n", stderr);
        return 1;
    }
    return 0;
}

#endif

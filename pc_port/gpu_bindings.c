#include "musashi_boot_memory.h"

/* This direct-call ABI has no guest CPU continuation. */
int musashi_boot_call_8005c1c0(MusashiBootMemory *memory,
                               const MusashiGpuDevice *device,
                               int32_t mode, int32_t *result) {
    (void)memory;
    (void)device;
    (void)mode;
    (void)result;
    return 0;
}

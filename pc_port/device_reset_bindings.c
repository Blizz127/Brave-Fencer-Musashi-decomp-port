#include "musashi_boot_memory.h"

/* This direct-call ABI cannot run guest instructions or deliver device IRQs.
 * Report unavailable before touching the device, caller result or guest RAM. */
int musashi_boot_call_8005bd7c(MusashiBootMemory *memory,
                               const MusashiGpuDevice *device,
                               int32_t mode, int32_t *result) {
    (void)memory;
    (void)device;
    (void)mode;
    (void)result;
    return 0;
}

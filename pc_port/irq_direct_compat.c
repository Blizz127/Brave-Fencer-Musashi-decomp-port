#include "musashi_irq_dispatch.h"
#include "musashi_callback_registration.h"

/* These historical bare-memory entry points have no installed HookEntryInt
 * record, exception owner or verified executable view. Only the scheduler
 * can enter the guest dispatcher and complete its nonlocal return. */
int musashi_boot_call_800427f4(MusashiBootMemory *memory, const MusashiIrqDispatchDevice *io) {
    (void)memory; (void)io; return 0;
}
int musashi_boot_call_80042ce8(MusashiBootMemory *memory, MusashiGuestCallbackExecutor execute,
                               void *userdata) {
    (void)memory; (void)execute; (void)userdata; return 0;
}
int musashi_boot_call_80042e08(MusashiBootMemory *memory,
    const MusashiDmaCallbackRegistrationDevice *device, MusashiGuestCallbackExecutor execute,
    void *userdata) {
    (void)memory; (void)device; (void)execute; (void)userdata; return 0;
}

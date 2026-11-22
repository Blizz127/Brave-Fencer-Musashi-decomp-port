/* Retain the live operand for the script worker and enqueue its request. */
#include "mine_script_queue.h"

void func_8017A094(const void *payload)
{
    u16 request = 0x15;
#if !(defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2)
    if ((u16)D_801A0BDC >= 512) return;
#endif
    D_801A0BFC = payload;
    func_80179B74(&request);
}

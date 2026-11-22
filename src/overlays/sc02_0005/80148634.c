#include "tower_handover.h"

s32 func_80148634(void *player)
{
    s32 centered = 0x8080;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    /* Keep the value shared between the final write and callback result. */
    __asm__("" : "=r"(centered) : "0"(centered));
#endif
    HANDOVER_SET_HALF(player, 0xaa, 0);
    HANDOVER_SET_HALF(player, 0xac, 0);
    HANDOVER_SET_HALF(player, 0xae, centered);
    return centered;
}

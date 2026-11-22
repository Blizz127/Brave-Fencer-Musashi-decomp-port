/* Clear digital input and center both stick axes. */
#include "forest_player_state.h"

s32 func_80148634(void *object)
{
    ForestPlayerState *actor = object;
    s32 centered = 0x8080;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    __asm__("" : "=r"(centered) : "0"(centered));
#endif
    actor->field_aa = 0;
    actor->field_ac = 0;
    actor->field_ae = centered;
    return centered;
}

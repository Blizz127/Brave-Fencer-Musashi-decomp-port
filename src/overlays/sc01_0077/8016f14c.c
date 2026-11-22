/* Deliver the actor's optional scene callback after applying its input policy. */
#include "forest_action.h"

void func_8016F14C(ForestActionPlayer *actor)
{
    u32 word = actor->scene_callback;
    u32 address = word & 0x80ffffffu;
    if (word != 0) {
        if (word & 0x01000000u)
            func_80148634(actor);
#if defined(__mips__) && !defined(__LP64__)
        ((void (*)(ForestActionPlayer *))address)(actor);
#else
        bfm_forest_player_call(address, actor);
#endif
    }
}

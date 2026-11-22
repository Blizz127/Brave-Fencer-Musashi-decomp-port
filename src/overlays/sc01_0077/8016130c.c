/* Choose the player transition for the combined contact flags. */
#include "../../../include/forest_player_state.h"

extern void func_801599A4(ForestPlayerState *actor);
extern void func_8015BDD0(ForestPlayerState *actor);
extern void func_8015BF48(ForestPlayerState *actor);

s32 func_8016130C(ForestPlayerState *actor, s32 contact)
{
    if (contact == 0 || contact == 0x8000) {
        func_801599A4(actor);
        func_8015BDD0(actor);
        return 1;
    }
    if (contact & 0x4000) {
        func_801599A4(actor);
        func_8015BF48(actor);
        return 1;
    }
    return 0;
}

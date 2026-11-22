/* Refresh the collision state when requested, then combine the two contact results. */
#include "../../../include/forest_player_state.h"

extern s32 func_8014E934(ForestPlayerState *actor);
extern s32 func_8014CC28(ForestPlayerState *actor);
extern s32 func_8014F3E8(ForestPlayerState *actor);

s32 func_801612B8(ForestPlayerState *actor, s32 flags)
{
    s32 contact;
    if (!(flags & 1))
        func_8014E934(actor);
    contact = func_8014CC28(actor);
    contact |= func_8014F3E8(actor);
    return contact;
}

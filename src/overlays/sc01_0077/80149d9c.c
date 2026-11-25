/* SPDX-License-Identifier: MIT */
#include "forest_player_state.h"

extern s32 func_80149E94(ForestPlayerState *actor);
extern s32 func_80149DD8(ForestPlayerState *actor);

/* The action selector gives the first transition test priority. */
s32 func_80149D9C(ForestPlayerState *actor)
{
    if (!func_80149E94(actor))
        return func_80149DD8(actor) != 0;
    return 1;
}

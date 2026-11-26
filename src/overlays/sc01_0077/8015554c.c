/* SPDX-License-Identifier: MIT */
/* Enter the ground sword action and run its current phase. */
#include "forest_player_locomotion.h"

extern void func_80155580(ForestPlayerState *actor);

void func_8015554C(ForestPlayerState *actor)
{
    func_80147078(actor, 5);
    func_80155580(actor);
}

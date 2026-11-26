/* SPDX-License-Identifier: MIT */
/* Enter the ground sword action and initialize the swing directly. */
#include "forest_player_locomotion.h"

extern void func_801555F4(ForestPlayerState *actor);

void func_80155518(ForestPlayerState *actor)
{
    func_80147078(actor, 5);
    func_801555F4(actor);
}

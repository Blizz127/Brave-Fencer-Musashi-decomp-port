#ifndef MUSASHI_FOREST_KIND4_ACTION_H
#define MUSASHI_FOREST_KIND4_ACTION_H

#include "forest_player_locomotion.h"
#include "forest_ground_orientation.h"

void func_8001382C(s16 yaw, const void *vector, s32 *result);
s32 func_80146CA0(ForestPlayerState *actor);
void func_80146E90(ForestPlayerState *actor, s32 value);
s32 func_80146E98(ForestPlayerState *actor);
s32 func_80147324(u16 sample);
s32 func_8014CC28(ForestPlayerState *actor);
s32 func_8014F3E8(ForestPlayerState *actor);
s32 func_8015BE94(ForestPlayerState *actor);
s32 func_8015C0C4(ForestPlayerState *actor);

#define PLAYER_MODEL_YAW(actor) BFM_GROUND_ATTACHMENT_HALF(FOREST_ORIENTATION_BYTES((actor)->model, 0x14, FOREST_ORIENTATION_SUBOBJECT), 0x12)
#endif

#ifndef BFM_SCENE_POSITION_SLOT_H
#define BFM_SCENE_POSITION_SLOT_H

#include "psx_types.h"

typedef struct ScenePositionSlot {
    u16 kind;
    u8 pad02[4];
    s16 x;
    u8 pad08[2];
    s16 y;
    u8 pad0c[2];
    s16 z;
    u8 pad10[16];
    u32 target;
    u8 pad24[32];
    s16 prevX, prevY, prevZ;
    u8 pad4a[2];
    u32 actor, first, second;
    u8 pad58[16];
} ScenePositionSlot;

extern ScenePositionSlot D_8011F9D0[20];

#endif

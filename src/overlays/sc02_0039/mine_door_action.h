#ifndef MINE_DOOR_ACTION_H
#define MINE_DOOR_ACTION_H

#include "psx_types.h"

typedef struct MineDoorActionPlayer {
    u8 reserved[0x16c];
    u16 action;
} MineDoorActionPlayer;

extern u32 D_800AE6B0;
extern void (*D_80180D78[])(MineDoorActionPlayer *player);

void func_80150B28(MineDoorActionPlayer *player);

#endif

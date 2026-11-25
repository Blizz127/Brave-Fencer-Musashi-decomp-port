#ifndef BFM_ROOM_TRANSFER_H
#define BFM_ROOM_TRANSFER_H

#include "scene_command.h"
#include "room_descriptor.h"

typedef struct {
    u32 queued;
    u32 field04;
    u32 pending;
    u8 pad0C[0xA];
    u8 parameter;
    u8 pad17[2];
    u8 world;
} RoomTransferQueue;

extern RoomTransferQueue D_800AE6A8;

void func_800D1754(void);
s32 func_800D17B8(u32 request);
void func_800D185C(u8 *descriptor);

#endif

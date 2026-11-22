#ifndef BFM_MINE_FRAME_SLOTS_H
#define BFM_MINE_FRAME_SLOTS_H

#include "psx_types.h"

typedef struct {
    u8 active;
    u8 handler;
    u8 reserved[30];
} MineFrameSlot;

extern MineFrameSlot D_80115218[4];
extern void (*D_801824C8[])(MineFrameSlot *);
void func_8016E95C(void);

#endif

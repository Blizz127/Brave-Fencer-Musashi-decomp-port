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
extern u16 D_801824BC[];
extern u8 D_801824AC[];
void func_8016EC0C(MineFrameSlot *slot, s32 shade);
void func_8016E8F0(void);
void func_8016E95C(void);
void func_8016E9EC(MineFrameSlot *slot);
void func_8016E918(s16 index, s16 value);
void func_8016EAC0(MineFrameSlot *slot);
void func_8016EB3C(MineFrameSlot *slot);
void func_8016EBA8(MineFrameSlot *slot);

#endif

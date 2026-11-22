#ifndef BFM_MINE_FRAME_STATE_H
#define BFM_MINE_FRAME_STATE_H

#include "psx_types.h"

typedef struct {
    u8 state;
    u8 reserved[3];
} MineFrameState;

typedef struct {
    u8 mode;
    u8 reserved[8];
    u8 flag;
} MineFrameMode;

extern MineFrameState D_8011F750;
extern MineFrameMode D_8011F7A8;
extern void (*D_80181CA0[])(MineFrameState *);
extern void (*D_801828E8[])(MineFrameMode *);
extern u8 D_800B9A13;
extern u32 D_80126B9C;
u32 func_80029504(void);
void func_801627E8(void);
void func_801754A8(void);

#endif

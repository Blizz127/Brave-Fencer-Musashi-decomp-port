#ifndef BFM_SC03_0116_ROOM_EVENT_H
#define BFM_SC03_0116_ROOM_EVENT_H

#include "psx_types.h"

typedef struct {
    u8 prefix[0x15];
    u8 phase;
} Bfm304cEvent;

typedef s32 (*Bfm304cEventStage)(Bfm304cEvent *event);
extern Bfm304cEventStage D_801862EC[2];

s32 func_80180994(Bfm304cEvent *event);
s32 func_801809D0(Bfm304cEvent *event);
void func_80180C1C(void);
void func_800167B8(s32 mode);

#endif

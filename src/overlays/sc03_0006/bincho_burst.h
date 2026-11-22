#ifndef BFM_BINCHO_BURST_H
#define BFM_BINCHO_BURST_H

#include "forest_field.h"

typedef struct BinchoReleaseConfig {
    u8 villager;
    u8 load_effect;
    u16 effect_id;
} BinchoReleaseConfig;

typedef struct BinchoBurstSprite {
    u8 pad00[0x18];
    s16 width;
    s16 height;
} BinchoBurstSprite;

typedef struct BinchoBurstActor {
    u16 type;
    u16 phase;
    u8 pad04[0x18];
    s32 timer;
    u8 pad20[0xac];
    u16 *outer_effect;
    BinchoBurstSprite *glow;
    u8 padD4[8];
    BinchoReleaseConfig *config;
    u8 padE0[0x28];
    s16 effect_handle;
} BinchoBurstActor;

extern u8 D_80126B58[];
extern void func_8014BCEC(void *, s32);
extern void func_8014BD24(void *, s32);
extern void func_8002D4C8(s32, s32);

#endif

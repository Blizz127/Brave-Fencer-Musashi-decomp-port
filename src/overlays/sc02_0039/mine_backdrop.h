#ifndef BFM_MINE_BACKDROP_H
#define BFM_MINE_BACKDROP_H

#include "psx_types.h"

typedef struct {
    s32 w[4];
} MineBackdropTransform;

typedef struct {
    s32 w[9];
} MineBackdropPrimitive;

typedef struct {
    s32 transform[4];
    u8 reserved10[12];
    u16 x;
    u16 y;
    u8 reserved20[20];
    u16 state;
    u16 flag;
    u16 off_x;
    u16 off_y;
    u16 reserved3c;
    u16 height;
    u16 base_x;
    u16 base_y;
    u8 reserved44[16];
} MineBackdropRecord;

typedef struct {
    u8 reserved[0xa3d2];
    u16 template_index;
} MineBackdropGlobals;

extern s16 D_80114EE0;
extern u8 D_801A0D70;
extern MineBackdropRecord D_800B9A78[2];
extern MineBackdropGlobals D_800AF630;
extern u8 D_8019F94C[];
#if defined(__mips__) && !defined(__LP64__)
extern u8 D_8019F950[];
#else
#define D_8019F950 (D_8019F94C + 4)
#endif
extern u8 D_800A6518[];
void func_801299C8(s16 selection, s16 index, void *record);
void func_80015B6C(s32 x, s16 y, s32 w, s16 h, s32 r0, s32 g0, s32 b0, s32 r1, s32 g1, s32 b1);
void func_8005080C(void *primitive, void *template_data, u16 state);
void func_80051044(void *primitive, void *template_data, u16 state);

#if !defined(__mips__) || defined(__LP64__)
extern MineBackdropPrimitive mine_backdrop_primitive;
extern MineBackdropTransform mine_backdrop_transform;
#endif

void func_8012956C(void);

#endif

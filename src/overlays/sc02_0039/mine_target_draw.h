#ifndef BFM_MINE_TARGET_DRAW_H
#define BFM_MINE_TARGET_DRAW_H

#include "psx_types.h"

typedef struct {
    s16 x, y, z, pad;
} TargetVertex;
typedef struct {
    u8 red, green, blue, pad;
} TargetColor;
typedef struct {
    TargetVertex vertex[4];
    TargetColor color[4];
    u32 tag;
} TargetPrimitive;

extern u8 D_801821E4[];
extern u8 D_801821F8[];
extern u16 D_800B99DA;
extern void func_80168070(u8 *object, s16 *from, s16 *to, void *out);
extern void func_80017758(void *primitive, void *matrix);

void func_80167AE0(u8 *object, s16 *from, s16 *to);

#endif

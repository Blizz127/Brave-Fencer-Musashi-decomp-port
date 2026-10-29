/* Owner re-derivation from retail assembly (clean input set). */
/* Builds an orientation matrix from a direction vector and sets a distance-based scale. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x12];
    /* 0x12 */ s16 angle12;
} Sub80162968;

typedef struct {
    u8 pad00[0x0A];
    /* 0x0A */ u16 base0A;
    u8 pad0C[0x20 - 0x0C];
    /* 0x20 */ u32 sub20;
    u8 pad24[0x44 - 0x24];
    /* 0x44 */ u32 flags44;
    u8 pad48[0x80 - 0x48];
    /* 0x80 */ s16 dir80;
    /* 0x82 */ s16 dir82;
    /* 0x84 */ s16 dir84;
} Src80162968;

typedef struct {
    u8 pad00[0x10];
    /* 0x10 */ s16 rotY;
    u8 pad12[0x14 - 0x12];
    /* 0x14 */ s16 rotX;
    u8 pad16[0x18 - 0x16];
    /* 0x18 */ s16 scaleX;
    /* 0x1A */ s16 scaleY;
    /* 0x1C */ s16 scaleZ;
} Dst80162968;

typedef struct {
    u8 pad00[0x04];
    /* 0x04 */ u32 src;
    /* 0x08 */ u32 dst;
    /* 0x0C */ u8 matrix[0x24 - 0x0C];
    /* 0x24 */ s32 value24;
} Obj80162968;

extern s32 func_8004CFEC(s32 y, s32 x);
extern s32 func_80047D3C(s32 v);
extern void func_80013F3C(void *m);
extern void func_800126C4(void *m, s32 angle);
extern void func_800123F0(void *m, s32 angle);
extern void func_80012558(void *m, s32 angle);

void func_80162968(Obj80162968 *obj)
{
    Src80162968 *src;
    Dst80162968 *dst;
    void *m;
    s16 scale;

    src = (Src80162968 *)obj->src;
    dst = (Dst80162968 *)obj->dst;
    m = obj->matrix;
    dst->rotY = func_8004CFEC(src->dir84, src->dir82);
    dst->rotX = func_8004CFEC(src->dir80,
        -(s16)func_80047D3C(src->dir84 * src->dir84 + src->dir82 * src->dir82));
    func_80013F3C(m);
    func_800126C4(m, dst->rotY);
    func_800123F0(m, dst->rotX);
    func_80012558(m, ((Sub80162968 *)src->sub20)->angle12);
    func_800126C4(m, 0x400);
    scale = ((0x300 - (s16)(obj->value24 - src->base0A)) << 13) / 0x300;
    if (src->flags44 & 0x400) {
        scale = scale / 3;
    }
    dst->scaleZ = scale;
    dst->scaleY = scale;
    dst->scaleX = scale;
}

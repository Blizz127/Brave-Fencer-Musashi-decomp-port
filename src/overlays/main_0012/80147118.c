/* Owner re-derivation from retail assembly (clean input set). */
/* Combines three 4.12 scale vectors, clamps each axis to 0xFFFF and writes it to the model scale. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u8 pad00[0x18];
    /* 0x18 */ s16 scale[3];
} Model80147118;

typedef struct {
    /* 0x00 */ u8 pad00[0x20];
    /* 0x20 */ u32 model; /* guest pointer */
    /* 0x24 */ u8 pad24[0x3C];
    /* 0x60 */ s16 scaleA[4];
    /* 0x68 */ s16 scaleB[4];
    /* 0x70 */ s16 scaleC[3];
} Obj80147118;

void func_80147118(Obj80147118 *obj)
{
    s32 s[3];

    s[0] = obj->scaleA[0] * obj->scaleB[0] / 4096;
    s[0] = s[0] * obj->scaleC[0] / 4096;
    s[1] = obj->scaleA[1] * obj->scaleB[1] / 4096;
    s[1] = s[1] * obj->scaleC[1] / 4096;
    s[2] = obj->scaleA[2] * obj->scaleB[2] / 4096;
    s[2] = s[2] * obj->scaleC[2] / 4096;
    if (s[0] > 0xFFFF) {
        s[0] = 0xFFFF;
    }
    if (s[1] > 0xFFFF) {
        s[1] = 0xFFFF;
    }
    if (s[2] > 0xFFFF) {
        s[2] = 0xFFFF;
    }
    ((Model80147118 *)obj->model)->scale[0] = s[0];
    ((Model80147118 *)obj->model)->scale[1] = s[1];
    ((Model80147118 *)obj->model)->scale[2] = s[2];
}

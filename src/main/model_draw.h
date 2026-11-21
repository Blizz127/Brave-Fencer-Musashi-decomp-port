#ifndef BFM_MODEL_DRAW_H
#define BFM_MODEL_DRAW_H

#include "psx_types.h"

/* Part draw request. Embedded addresses keep the guest's 32-bit layout. */
typedef struct {
    u32 attr;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
    u32 unk14;
} DrawArg;

/* Per-buffer ordering-table context. */
typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
} OtRec;

void func_800243EC(DrawArg *arg, OtRec *ot, s32 mode);
void func_80024448(DrawArg *arg, OtRec *ot, s32 mode);
void func_80026D64(DrawArg *arg, OtRec *ot, s32 mode);

#endif

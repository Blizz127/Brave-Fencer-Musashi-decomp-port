/* Owner re-derivation from retail assembly (clean input set). */
/* Rebuilds the model matrix: identity, rotate by angles 0x100..0x104, then by overrides or the model's own angles. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u8 pad00[0x10];
    /* 0x10 */ s16 rot[3];
    /* 0x16 */ u8 pad16[0x1E];
    /* 0x34 */ s16 matrix[16];
} Model80147628;

typedef struct {
    /* 0x000 */ u8 pad000[0x20];
    /* 0x020 */ u32 model; /* guest pointer: Model80147628 */
    /* 0x024 */ u8 pad024[0xDC];
    /* 0x100 */ s16 angle[3];
    /* 0x106 */ u8 pad106[0x2A];
    /* 0x130 */ s16 overrideRot[3];
    /* 0x136 */ u16 overrideMask;
} Obj80147628;

extern void func_80013F3C(s16 *m);
extern void func_80012558(s16 *m, s16 angle);
extern void func_800126C4(s16 *m, s16 angle);
extern void func_800123F0(s16 *m, s16 angle);

void func_80147628(Obj80147628 *obj)
{
    Model80147628 *model;
    s16 *m;

    model = (Model80147628 *)obj->model;
    m = model->matrix;
    func_80013F3C(m);
    func_80012558(m, obj->angle[1]);
    func_800126C4(m, obj->angle[0]);
    func_800123F0(m, obj->angle[2]);
    if (obj->overrideMask & 2) {
        func_80012558(m, obj->overrideRot[1]);
    } else {
        func_80012558(m, model->rot[1]);
    }
    if (obj->overrideMask & 1) {
        func_800126C4(model->matrix, obj->overrideRot[0]);
    } else {
        func_800126C4(model->matrix, model->rot[0]);
    }
    if (obj->overrideMask & 4) {
        func_800123F0(model->matrix, obj->overrideRot[2]);
    } else {
        func_800123F0(model->matrix, model->rot[2]);
    }
}

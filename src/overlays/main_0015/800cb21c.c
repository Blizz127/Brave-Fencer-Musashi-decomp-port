/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Spawns the child record (func_8014659C) and sets it up from the model at +0x4C; without one, func_80162CCC. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x4];
    u32 flags;          /* 0x4 */
    u8 pad8[0x10 - 0x8];
    u16 f10;            /* 0x10 */
    u16 f12;            /* 0x12 */
    u16 f14;            /* 0x14 */
} Child;

typedef struct {
    u8 pad0[0x12];
    u16 f12;            /* 0x12 */
} Ref;

typedef struct {
    u8 pad0[0x20];
    u32 ref;            /* 0x20: guest address of a Ref */
    u8 pad24[0x100 - 0x24];
    u16 f100;           /* 0x100 */
    u16 pad102;
    u16 f104;           /* 0x104 */
} Model;

typedef struct {
    u8 pad0[0x4];
    s32 pos[3];         /* 0x4 */
    u8 pad10[0x20 - 0x10];
    u32 child;          /* 0x20: guest address of a Child */
    u8 pad24[0x4C - 0x24];
    u32 model;          /* 0x4C: guest address of a Model */
} Obj;

extern void *func_8014659C(void);
extern void func_8001C2C4(void *p);
extern void func_80149374(void *o, void *x);
extern void func_80146DE8(void *p, s32 a, s32 b, s32 c);
extern void func_80146E90(void *o, s32 v);
extern void func_80146CA0(void *o);
extern void func_80162CCC(void *o);

void func_800CB21C(Obj *o)
{
    Model *model = (Model *)o->model;
    Child *c;

    c = func_8014659C();
    o->child = (u32)c;
    if (c != 0) {
        func_8001C2C4(c);
        c->flags |= 0x80000000;
        func_80149374(model, o->pos);
        c->f10 = model->f100;
        c->f14 = model->f104;
        c->f12 = ((Ref *)model->ref)->f12;
        func_80146DE8(o, 0, 0, (s32)0xFFC00000);
        func_80146E90(o, 0x10);
        func_80146CA0(o);
    } else {
        func_80162CCC(o);
    }
}

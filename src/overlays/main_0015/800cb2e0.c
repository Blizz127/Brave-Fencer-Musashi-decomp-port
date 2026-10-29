/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Places the effect: at once when func_80146E98 says so, else builds a rotation from the child's angles and picks colour set D_80114EB0 or D_80114EC8 by func_801632F0. */
#include "psx_types.h"

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } Mat;

typedef struct {
    u8 pad0[0x10];
    s16 f10;            /* 0x10 */
    s16 f12;            /* 0x12 */
    s16 f14;            /* 0x14 */
} Child;

typedef struct { u16 lo; s16 hi; } Fx;

typedef struct {
    u16 pad0;
    u16 state;          /* 0x2 */
    Fx pos[3];          /* 0x4 */
    u8 pad10[0x20 - 0x10];
    u32 child;          /* 0x20: guest address of a Child */
} Obj;

extern s16 D_80126B18;
extern s16 D_80126B1A;
extern s16 D_80126B1C;
extern u8 D_80114EB0[];
extern u8 D_80114EC8[];
extern s32 func_80146E98(void *o);
extern s32 func_80146A6C(s32 a, void *o, s32 c, s32 d, s32 e, s32 f, s32 g);
extern void func_80146CA0(void *o);
extern void func_80013F3C(void *m);
extern void func_800126C4(void *m, s32 a);
extern void func_800123F0(void *m, s32 a);
extern void func_80012558(void *m, s32 a);
extern void func_80163194(void *o, s32 a, s32 b, s32 c, void *d);
extern void func_80162FC0(void *o);
extern void func_80163408(void *o, s32 a, s32 b, s32 c);
extern void func_80163328(void *o);
extern u32 func_801632F0(void *o);
extern void func_80013E94(void *src, void *dst);

void func_800CB2E0(Obj *o)
{
    Child *c = (Child *)o->child;
    Mat spare;          /* unused; the retail frame keeps a 32-byte slot below m (sp+0x20) */
    Mat m;
    u32 w;

    (void)spare;
    if (func_80146E98(o) != 0) {
        func_80146A6C(3, o, o->pos[0].hi, o->pos[1].hi, o->pos[2].hi, 0, (s32)0x80001C00);
        func_80146CA0(o);
    } else {
        func_80013F3C(&m);
        func_800126C4(&m, c->f10);
        func_800123F0(&m, c->f14);
        func_80012558(&m, c->f12);
        func_80163194(o, 0, 0, 0, &m);
        func_80162FC0(o);
        func_80163408(o, 0x10, 0x80, 0x200);
        func_80163328(o);
        switch (func_801632F0(o)) {
        case 1:
            func_80013E94(D_80114EB0, &w);
            w |= 0x80000000;
            func_80146A6C(0x1F, o, D_80126B18, D_80126B1A, D_80126B1C, w, 0);
            func_80146CA0(o);
            break;
        case 2:
        case 4:
            func_80013E94(D_80114EC8, &w);
            w |= 0x80000000;
            func_80146A6C(7, o, D_80126B18, D_80126B1A, D_80126B1C, w, 0);
            func_80146CA0(o);
            break;
        }
    }
}

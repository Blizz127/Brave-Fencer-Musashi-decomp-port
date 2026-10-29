/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Per-frame ability update: kind 'A' eases the child's 0x12 value, 'S'/'s' run func_80148AFC; mode bits 0x4000/0x8000 start effects and the next animation. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    s16 f12;            /* 0x12 */
} Child;

typedef struct {
    u8 pad0[0x20];
    u32 child;          /* 0x20: guest address of a Child */
    u8 pad24[0x44 - 0x24];
    u32 flags;          /* 0x44 */
    u8 pad48[0xA9 - 0x48];
    u8 kind;            /* 0xA9 */
    u8 padAA[0xB8 - 0xAA];
    u16 mode;           /* 0xB8 */
    u8 padBA[0x197 - 0xBA];
    u8 step;            /* 0x197 */
} Obj;

extern u8 D_800CBAE0[];
extern u8 D_800CBB00[];
extern u8 D_80062BD0[];
extern s32 func_80161208(void *o);
extern s16 func_80148E54(void *o);
extern s32 func_80012ABC(s32 from, s16 to, s16 rate);
extern void func_80148AFC(void *o);
extern void func_801466F0(s32 a, void *o, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h);
extern s32 func_80146A6C(s32 a, void *o, s32 c, s32 d, s32 e, s32 f, s32 g);
extern void func_80147324(u16 a);
extern void func_801655E4(void *o);
extern void func_80019064(s32 a0);
extern s32 func_80165624(s32 o);
extern u16 func_801487F4(void *o);
extern void func_80154274(void *o, u32 v);

void func_800CAF9C(Obj *o)
{
    Child *c = (Child *)o->child;
    s16 v;

    o->flags |= 2;
    if (func_80161208(o) != 0) {
        return;
    }
    switch (o->kind) {
    case 0x41:
        v = func_80148E54(o);
        if (v > 0) {
            c->f12 = func_80012ABC(c->f12, v, 4);
        }
        break;
    case 0x53:
    case 0x73:
        func_80148AFC(o);
        break;
    }
    if (o->mode & 0x4000) {
        func_801466F0(2, o, 0, -0x18, -0xF, 0, 0, 1);
        func_80146A6C(0xF, o, 0, -0x18, -0xF, 0, 0);
        func_80147324(0x476);
        func_801655E4(o);
        func_80019064((s32)D_80062BD0);
    }
    if (o->mode & 0x8000) {
        if (func_80165624((s32)o) == 0 && (func_801487F4(o) & 0x20)) {
            func_80154274(o, (u32)D_800CBB00);
        } else {
            func_80154274(o, (u32)D_800CBAE0);
            o->step++;
        }
    }
}

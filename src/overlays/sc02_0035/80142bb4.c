/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    s32 f4; /* 0x4 */
} Sub;

typedef struct {
    u8 pad0[0x20];
    Sub *f20;   /* 0x20 */
    u8 pad24[0x5C - 0x24];
    u16 f5C;    /* 0x5C */
    u8 pad5E[0xCC - 0x5E];
    s32 fCC;    /* 0xCC */
    u8 padD0[0xFC - 0xD0];
    s16 fFC;    /* 0xFC */
    u8 pad_fe[0x100 - 0xFE];
    s16 f100;   /* 0x100 */
} Obj;

extern void func_801439C0(s32 a0);
extern void func_8012AD44(Obj *o, s32 a);
extern void func_8012C218(Obj *o);
extern void func_8012C098(Obj *o);

void func_80142BB4(Obj *o, s32 a1, s32 a2) {
    s32 s1;

    s1 = a2;
    if (o->fFC == 1) {
        if (a1 == 0 && o->f100 != 0) {
            return;
        }
        {
            u16 v0;
            Sub *sub;

            v0 = o->f5C;
            sub = o->f20;
            v0 = v0 & 0xFEFE;
            o->f5C = v0;
            sub->f4 = sub->f4 | 0x80000000;
        }
        func_801439C0(o->fCC);
        func_8012AD44(o, (s16)s1);
        return;
    }

    if (a1 == 0 && o->f100 != 0) {
        goto call_c098;
    }
    func_8012C218(o);
    return;
call_c098:
    func_8012C098(o);
}

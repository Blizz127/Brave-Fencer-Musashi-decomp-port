/* func_80149954 [80149954,80149A64) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x44];
    u32 f44;
    u8 p48[0x62];
    u16 fAA;
    u16 fAC;
} O;

typedef struct {
    s16 a, b, c, d;
    u32 e, f, g;
} R;

/* callers read v0: yes (tail callers return it); returns 1 after queueing the record, else 0.
 * func_8012E5CC takes the local vector (owner: V *), declared void * here. */
extern void func_80015978(void *src, void *dst);
extern void func_8012E5CC(void *v, s32 a, s32 b);
extern void func_8002D4C8(s32 a, s32 b);
extern void func_80147364(s32 a, s32 b);
extern void func_800CCF28(O *o);
extern void func_80146750(R *r);
extern s16 D_800B9A08;
extern s32 D_8012707C;
extern s32 D_80126B5C;

s32 func_80149954(O *o) {
    u16 v[4];
    R r;

    if (o->f44 & 0x404) {
        return 0;
    }
    if ((o->fAA & 0x90) == 0) {
        if (D_800B9A08 == 0x308D) {
            func_80015978(((s32 *)&D_80126B5C), v);
            func_8012E5CC(v, 4, 0x8E0);
            return 0;
        }
        func_8002D4C8(4, 0x8E0);
        return 0;
    }
    if ((o->fAC & 0x90) != 0) {
        if (D_8012707C == 0x8E0) {
            D_8012707C = 0;
        } else {
            func_80147364(0x8E0, 0);
        }
    }
    func_800CCF28(o);
    r.a = 0x12;
    r.e = (u32)o;
    r.b = 0;
    r.c = -0x1E;
    r.d = 0;
    r.f = 0;
    r.g = 0;
    func_80146750(&r);
    return 1;
}

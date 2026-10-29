/* Owner re-derivation from retail assembly (clean input set). */
/* If o->f70==0 and o->f64 chains to a non-null target (o->f64->+0x20), reads that target's
 * +0x12 angle, normalizes it, and uses func_80047948/func_8004787C (sin/cos-style lookups)
 * scaled by D_80185DFC to set o->f10/f18 velocity components, then runs func_8012AD80(o) to
 * integrate. Either way, mirrors o->f6/fA/fE into the target o->fCC and steps its animator via
 * func_80128ED8; if that finishes, runs func_80016714(target,0x38); func_8012C218(o). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x6];
    u16 f6;   /* 0x6 */
    u8 pad8[0xA - 0x8];
    u16 fA;   /* 0xA */
    u8 padC[0xE - 0xC];
    u16 fE;   /* 0xE */
    u8 pad10[0x64 - 0x10];
    u8 *f64;  /* 0x64 */
    u8 pad68[0x70 - 0x68];
    s16 f70;  /* 0x70 */
    u8 pad72[0xCC - 0x72];
    void *fCC; /* 0xCC */
} Obj;

typedef struct {
    u8 pad0[8];
    u16 f8;  /* 0x8 */
    u16 fA;  /* 0xA */
    u16 fC;  /* 0xC */
} Target;

extern s32 func_80047948(s32 angle);
extern s32 func_8004787C(s32 angle);
extern s32 D_80185DFC;
extern void func_8012AD80(void *o);
extern s32 func_80128ED8(void *o, void *st);
extern void func_80016714(void *o, s32 a);
extern void func_8012C218(void *o);

void func_80144458(Obj *o)
{
    Target *s2 = (Target *)o->fCC;

    if (o->f70 == 0) {
        u8 *p = o->f64;
        u8 *q = *(u8 **)(p + 0x20);
        if (q != 0) {
            s16 angle = *(s16 *)(q + 0x12);
            s32 a = angle;
            a -= 0x400;
            a &= 0xFFF;

            *(u32 *)((u8 *)o + 0x10) = (func_80047948(a) * D_80185DFC) << 4;
            *(u32 *)((u8 *)o + 0x18) = (func_8004787C(a) * (-D_80185DFC)) << 4;
            func_8012AD80(o);
        }
    }

    s2->f8 = o->f6;
    s2->fA = o->fA;
    s2->fC = o->fE;

    if (func_80128ED8(s2, (u8 *)o + 0xD0) != 0) {
        func_80016714(s2, 0x38);
        func_8012C218(o);
    }
}

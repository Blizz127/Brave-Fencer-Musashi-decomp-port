/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x30];
    s16 f30;
} Target;

typedef struct {
    u8 pad0[6];
    u16 f6;       /* 0x6 */
    u8 pad8[2];
    u16 fA;       /* 0xA */
    u8 padC[2];
    u16 fE;       /* 0xE */
    u8 pad10[0x5E - 0x10];
    u8 f5E;       /* 0x5E */
    u8 pad5F[1];
    s16 f60;      /* 0x60 */
    u8 pad62[0x78 - 0x62];
    Target *f78;  /* 0x78 */
    u16 f7C;      /* 0x7C */
    u16 f7E;      /* 0x7E */
    u16 f80;      /* 0x80 */
    u16 f82;      /* 0x82 */
} Obj;

extern void func_8016AA50(Obj *o, s32 a1);
extern void func_8016B428(Obj *o);
extern void func_80019064(void *a0);
extern u8 D_80184824[];

void func_8012E9C0(Obj *o) {
    s32 a1;

    if (o->f60 == 0) {
        return;
    }
    if (o->f5E == 0x1D) {
        o->f82 = 0;
        o->f7C = o->f6;
        o->f7E = o->fA;
        o->f80 = o->fE;
    }
    if (o->f78 != 0) {
        s32 prod;

        prod = o->f60 * o->f78->f30;
        a1 = prod >> 12;
        if (a1 <= 0) {
            a1 = 1;
        }
    } else {
        a1 = o->f60;
    }
    func_8016AA50(o, a1);
    if (o->f82 & 1) {
        func_8016B428(o);
        func_80019064(D_80184824);
    }
}

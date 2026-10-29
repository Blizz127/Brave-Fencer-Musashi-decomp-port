/* Owner re-derivation from retail assembly (clean input set). */
/* Extended twin of the already-matched sibling func_8012E9C0 (same overlay): adds
 * the f5C busy-flag clear, the fC8/fC9 callout pair, the f76 sign check and the
 * final f20 sub-record reset + func_8012B2CC/func_8012B23C calls. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x30];
    s16 f30;
} Target;

typedef struct {
    u8 pad0[0x10];
    u16 f10; /* 0x10 */
    u16 f12; /* 0x12 */
    u16 f14; /* 0x14 */
} Sub;

typedef struct {
    u8 pad0[6];
    u16 f6;        /* 0x6 */
    u8 pad8[2];
    u16 fA;        /* 0xA */
    u8 padC[2];
    u16 fE;        /* 0xE */
    u8 pad10[0x20 - 0x10];
    Sub *f20;      /* 0x20 */
    u8 pad24[0x5C - 0x24];
    u16 f5C;       /* 0x5C */
    u8 pad5E[0x5E - 0x5E];
    u8 f5E;        /* 0x5E */
    u8 pad5F[1];
    s16 f60;       /* 0x60 */
    u8 pad62[0x62 - 0x62];
    u16 f62;       /* 0x62 */
    u8 pad64[0x76 - 0x64];
    s16 f76;       /* 0x76 */
    u8 pad78[0x78 - 0x78];
    Target *f78;   /* 0x78 */
    u16 f7C;       /* 0x7C */
    u16 f7E;       /* 0x7E */
    u16 f80;       /* 0x80 */
    u16 f82;       /* 0x82 */
    u8 pad84[0xC8 - 0x84];
    u8 fC8;        /* 0xC8 */
    u8 fC9;        /* 0xC9 */
} Obj;

extern void func_8016AA50(Obj *o, s32 a1);
extern void func_8016B428(Obj *o);
extern void func_80019064(void *a0);
extern void func_8002A520(Obj *o);
extern void func_8002A790(Obj *o);
extern void func_8012B2CC(Obj *o);
extern void func_8012B23C(Obj *o);
extern u8 D_80184990[];

void func_80131B14(Obj *o, s32 a1) {
    s32 f5E;

    f5E = o->f5E;
    if (o->f60 != 0) {
        if (f5E == 0x1D) {
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
        }
        o->f76 = o->f76 - a1;
        func_8016AA50(o, a1);
        if (o->f82 & 1) {
            func_8016B428(o);
            func_80019064(D_80184990);
        }
    }

    o->f5C = o->f5C & 0xFFFE;
    if (f5E != 0x1D) {
        if (o->fC8 != 0) {
            func_8002A520(o);
        }
        if (o->fC9 != 0) {
            func_8002A790(o);
        }
    }
    if (o->f76 <= 0) {
        o->f5C = 0;
    }

    {
        Sub *sub1;
        Sub *sub2;

        sub1 = o->f20;
        sub1->f12 = (o->f62 + 0x800) & 0xFFF;
        sub2 = o->f20;
        sub2->f14 = 0;
        sub2->f10 = 0;
    }
    func_8012B2CC(o);
    func_8012B23C(o);
}

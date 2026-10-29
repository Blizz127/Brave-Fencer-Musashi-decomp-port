/* func_8016B114 [8016B114,8016B234) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x24];
    u8 f24;
    u8 f25;
    u8 f26;
} S;

typedef struct {
    u8 p0[0xA];
    u16 fA;
    u8 pC[8];
    u32 f14;
    u8 p18[4];
    u32 f1C;
    S *f20;
    u16 f24;
    u16 f26;
    u8 p28[4];
    u32 f2C;
} O;

/* callers read v0: none-direct (no jal caller); void. f20 holds a guest address.
 * func_8016B234 receives o (retail leaves a0 = o; its body saves it). */
extern void func_8016B234(O *o);
extern void func_800D22E4(O *o);
extern void func_80146C3C(O *o);

void func_8016B114(O *o) {
    S *s;

    s = o->f20;
    if (o->f2C < 6) {
        func_8016B234(o);
    }
    switch (o->f2C) {
    case 6:
    case 7:
        s->f24 = s->f25 = s->f26 = s->f26 - 0x10;
        o->fA = o->fA - 2;
        break;
    case 1:
    case 5:
        func_800D22E4(o);
        o->f14 += 0x18000;
        o->f26++;
        o->f24 -= 0x10;
        break;
    default:
        o->fA -= 2;
        o->f26++;
        o->f24 -= 0x10;
        break;
    }
    o->f1C = o->f1C - 1;
    if (o->f1C == 0) {
        func_80146C3C(o);
    }
}

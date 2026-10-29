/* func_8014B5D8 [8014B5D8,8014B6E8) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u8 p2[0x42];
    u32 f44;
} O;

typedef struct {
    u8 p0[0x24];
    s32 f24;
    s32 f28;
    u8 p2C[0x17];
    u8 f43;
    u8 p44[5];
    u8 f49;
} G;

/* callers read v0: none-direct (no jal caller); void.
 * func_8016F1C4 ignores a0 (owner prototype s32 f(void)). */
extern s32 func_8016F1C4(void);
extern u8 func_80029178(s32 a);
extern void func_8014BC80(O *o, s32 a);
extern u8 D_80078E78[];

void func_8014B5D8(O *o) {
    G *g;
    s32 a;
    s32 v;

    g = ((G *)&D_80078E78);
    if (func_8016F1C4() != 0) {
        return;
    }
    if (o->f0 == 0x1E || o->f0 == 0x1A) {
        return;
    }
    if (o->f44 & 0x10) {
        return;
    }
    a = 0x16C;
    if (o->f44 & 0x80) {
        if (func_80029178(0x1B) != 0) {
            a = 0x2D8;
        } else {
            a = 0x5B0;
        }
    }
    if (g->f49 == 0x12) {
        a = a << 1;
    }
    v = g->f24 - a;
    g->f24 = v;
    if (v < 0) {
        g->f24 = 0;
        if (o->f44 & 0x80) {
            g->f28 = g->f28 + 0xAAA8;
        } else {
            g->f28 = g->f28 + 0x1555;
        }
        if (((s16 *)&g->f28)[1] >= 0x3C) {
            g->f28 = 0;
            func_8014BC80(o, 4);
        }
    } else {
        g->f28 = 0;
        g->f43 = 0;
    }
}

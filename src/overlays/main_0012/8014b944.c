/* Owner re-derivation from retail assembly (clean input set). */
/* Adds step to two 16.16 values (a global at +0x2C of the block and the +0x60 field), clamping the integer part to 0x80/0xA0 depending on flag and the 0x1B button. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x60];
    union {
        u32 w;
        s16 h[2];
    } f60;
} G;

extern u8 D_80078E78[];
extern u32 D_80078EA4;
extern s16 D_80078EA6;
extern u8 func_80029178(s32 a);

void func_8014B944(s32 unused, s32 step, s32 flag) {
    G *g = ((G *)&D_80078E78);
    if (flag != 0) {
        D_80078EA4 += step;
        if (*(s16 *)((u8 *)&D_80078EA4 + 2) > 0xA0) {
            D_80078EA4 = 0xA00000;
        }
    } else if (func_80029178(0x1B) != 0) {
        D_80078EA4 += step;
        if (*(s16 *)((u8 *)&D_80078EA4 + 2) > 0xA0) {
            D_80078EA4 = 0xA00000;
        }
    } else if (D_80078EA6 < 0x80) {
        D_80078EA4 += step;
        if (*(s16 *)((u8 *)&D_80078EA4 + 2) > 0x80) {
            D_80078EA4 = 0x800000;
        }
    }
    if (flag != 0) {
        g->f60.w += step;
        if (g->f60.h[1] > 0xA0) {
            g->f60.w = 0xA00000;
        }
    } else if (func_80029178(0x1B) != 0) {
        g->f60.w += step;
        if (g->f60.h[1] > 0xA0) {
            g->f60.w = 0xA00000;
        }
    } else if (g->f60.h[1] < 0x80) {
        g->f60.w += step;
        if (g->f60.h[1] > 0x80) {
            g->f60.w = 0x800000;
        }
    }
}

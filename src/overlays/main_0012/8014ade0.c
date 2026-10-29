/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame input/timer step for the area block: advances the 16.16 clock, and on each new second dispatches the move/pause handlers. */
#include "psx_types.h"

typedef struct {
    u8 p0[0x18];
    union {
        s32 w;
        s16 h[2];
    } f18;
    u8 p1C[0x3C - 0x1C];
    u16 u3C;
    u8 p3E[2];
    u16 u40;
    u8 p42[7];
    u8 b49;
} G;

extern u8 D_80078E78[];
extern s32 D_80078E90;
extern u8 func_80029178(s32 a);
extern void func_8014B084(void);
extern u32 func_8014B154(void *o);
extern void func_8014BB24(void *o, s32 a, s32 b);
extern void func_8014BC80(void *o, s32 a);
extern void func_8014BD24(void *o, s32 a);
extern void func_8014BD60(void *o, s32 a);
extern s32 func_8016F1C4(void);

void func_8014ADE0(u8 *o) {
    G *g = ((G *)&D_80078E78);
    s32 old;
    s32 b;
    s16 t;
    s16 u;
    if (func_8016F1C4() == 0 && *(u16 *)o != 0x1A && *(u16 *)o != 0x1E && !(*(u32 *)(o + 0x44) & 0x10)) {
        old = D_80078E90;
        if (func_8014B154(o) != 0) {
            D_80078E90 += 0xAAA8;
        } else {
            D_80078E90 += 0x1555;
        }
        if (g->f18.h[1] - (old >> 16) > 0 && g->b49 == 3) {
            func_8014BD24(o, 1);
        }
        t = g->f18.h[1];
        u = old >> 16;
        if ((s16)(t / 60) - (s16)(u / 60) > 0) {
            if (func_8014B154(o) != 0) {
                if (func_80029178(0x1B) != 0) {
                    b = 0xA;
                } else {
                    b = 4;
                }
            } else {
                b = 4;
            }
            if (g->u40 != 0) {
                func_8014BB24(o, b, 0);
            } else if (g->u3C != 0) {
                if (g->u3C >= 5) {
                    func_8014BC80(o, 4);
                } else {
                    g->u3C = 1;
                }
            }
            if (func_8014B154(o) != 0) {
                if (func_80029178(0x1B) != 0) {
                    func_8014BD24(o, 8);
                } else {
                    func_8014BD60(o, 1);
                }
            } else {
                func_8014BD60(o, 4);
            }
        }
        if (g->f18.h[1] >= 0x5A0) {
            g->f18.w = 0;
            func_8014B084();
        }
    }
}

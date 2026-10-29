/* func_8014B350 [8014B350,8014B4C4) - owner re-derivation from retail assembly (clean input set).
 * Advances the two area timers (fast step 0xAAA8 when o->f44 bit 7, else 0x1555) and fires the
 * timeout effects. Each select is written as `X += K` in both arms (jump2 cross-jumps the store);
 * a shared result local made it a global pseudo with the constant/value registers swapped.
 * Callers read v0: none (void). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1C];
    union { u32 w; struct { u16 lo; s16 hi; } h; } f1C;
    u8 pad20[0x34];
    union { s32 w; struct { u16 lo; s16 hi; } h; } f54;
} G;
typedef struct {
    u16 id;
    u8 pad2[4];
    u16 f6;
    u8 pad8[2];
    u16 fA;
    u8 padC[2];
    u16 fE;
    u8 pad10[0x34];
    u32 f44;
} Obj;
typedef struct { u16 x, y, z, w; } V8;

extern u8 D_80078E78[];
extern s32 D_80078E94;
extern s16 D_80078E96;
extern u8 D_80062BF4[];
extern s32 func_8016F1C4(void);
extern void func_8014BC80(Obj *o, s32 a);
extern void func_8014AD30(Obj *o, V8 *v, s32 b, s32 c);
extern void func_80019064(void *p);

void func_8014B350(Obj *o) {
    V8 a;
    u16 t;
    G *g;

    g = ((G *)&D_80078E78);
    if (func_8016F1C4() == 0) {
        t = o->id;
        if (t != 0x1E && t != 0x1A) {
            if (D_80078E96 & 0x8000) {
                if (o->f44 & 0x80) {
                    D_80078E94 += 0xAAA8;
                } else {
                    D_80078E94 += 0x1555;
                }
                if ((g->f1C.h.hi & 0x7FFF) >= 0x3D) {
                    g->f1C.w = 0x80000000;
                    func_8014BC80(o, 8);
                    a.x = o->f6;
                    a.y = o->fA - 0x20;
                    a.z = o->fE;
                    func_8014AD30(o, &a, 6, 0);
                    func_80019064(D_80062BF4);
                }
            }
            if (g->f54.h.hi & 0x8000) {
                if (o->f44 & 0x80) {
                    g->f54.w += 0xAAA8;
                } else {
                    g->f54.w += 0x1555;
                }
                if ((g->f54.h.hi & 0x7FFF) >= 0x3D) {
                    g->f54.w = 0x80000000;
                }
            }
        }
    }
}

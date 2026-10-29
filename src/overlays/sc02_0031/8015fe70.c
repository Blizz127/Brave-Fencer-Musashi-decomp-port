/* Owner re-derivation from retail assembly (clean input set). */
/* If o->fB8 bit 0x8000 is set: when fBA is also set, copies it straight into fB8;
 * otherwise dispatches D_8018730C[o->f0](o) (discarding its result), then
 * func_80147078(o,0); func_80159B70(o); and returns. Otherwise (or after the fBA copy)
 * falls through to the common tail: func_80154A74(o,0x11) when fB8 bit 0x4000 is set,
 * then always func_80161208(o). Same Obj layout as the already-matched siblings
 * func_8015F9A4/func_8015DBD4. void, no caller found in any region. */
#include "psx_types.h"

typedef struct {
    u16 f0;  /* 0x0 */
    u8 pad2[0xB8 - 0x2];
    u16 fB8; /* 0xB8 */
    u16 fBA; /* 0xBA */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_8018730C[];
extern void func_80147078(Obj *o, s32 a);
extern void func_80159B70(Obj *o);
extern void func_80154A74(Obj *o, s32 a);
extern void func_80161208(Obj *o);

void func_8015FE70(Obj *o)
{
    if (o->fB8 & 0x8000) {
        if (o->fBA == 0) {
            ((Handler)D_8018730C[o->f0])(o);
            func_80147078(o, 0);
            func_80159B70(o);
            return;
        }
        o->fB8 = o->fBA;
    }

    if (o->fB8 & 0x4000) {
        func_80154A74(o, 0x11);
    }
    func_80161208(o);
}

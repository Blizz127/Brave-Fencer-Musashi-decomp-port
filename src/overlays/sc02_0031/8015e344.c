/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_8015E40C(o); then bails early (no further work) if func_80161B18(o),
 * func_80161B84(o) or func_80161208(o) return nonzero. Otherwise: if o->fB8&0x4000, runs
 * func_8015E5B0(o) and sets o->f234=1; if o->fB8&0x8000, dispatches D_8018730C[o->f0](o)
 * (discarded) then func_80147078(o,0); func_80159BE4(o). void, no caller found in any
 * region. */
#include "psx_types.h"

typedef struct {
    u16 f0;  /* 0x0 */
    u8 pad2[0xB8 - 0x2];
    u16 fB8; /* 0xB8 */
    u8 pad1[0x234 - 0xBA];
    s32 f234; /* 0x234 */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_8018730C[];
extern void func_8015E40C(Obj *o);
extern s32 func_80161B18(u8 *o);
extern s32 func_80161B84(u8 *o);
extern s32 func_80161208(Obj *o);
extern void func_8015E5B0(u8 *o);
extern void func_80147078(Obj *o, s32 a);
extern void func_80159BE4(u8 *o);

void func_8015E344(Obj *o)
{
    func_8015E40C(o);

    if (func_80161B18((u8 *)o) != 0) {
        return;
    }
    if (func_80161B84((u8 *)o) != 0) {
        return;
    }
    if (func_80161208(o) != 0) {
        return;
    }

    if (o->fB8 & 0x4000) {
        func_8015E5B0((u8 *)o);
        o->f234 = 1;
    }

    if (o->fB8 & 0x8000) {
        ((Handler)D_8018730C[o->f0])(o);
        func_80147078(o, 0);
        func_80159BE4((u8 *)o);
    }
}

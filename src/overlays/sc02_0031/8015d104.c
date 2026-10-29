/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_80149FB0(o); func_8014FA04(o); if func_8014E6A0(o) returns 0, dispatches
 * D_8018730C[o->f0](o) (discarding its result) then func_80147078(o,0); func_80159B70(o)
 * and returns. Otherwise runs func_801725F4(o), arms o->sub (f20) for a 0x600 effect
 * (o->f234 = 0xFF1D0000, sub->f10 = 0x600), reads that back and stores it at o->f244
 * alongside func_80146CA0(o). void, no caller found in any region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x10];
    u16 f10; /* 0x10 */
} Sub;

typedef struct {
    u16 f0;  /* 0x0 */
    u8 pad2[0x20 - 0x2];
    Sub *f20; /* 0x20 */
    u8 pad24[0x234 - 0x24];
    s32 f234; /* 0x234 */
    u8 pad238[0x244 - 0x238];
    u16 f244; /* 0x244 */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_8018730C[];
extern s32 func_80149FB0(Obj *o);
extern s32 func_8014FA04(Obj *o);
extern s32 func_8014E6A0(Obj *o);
extern void func_80147078(Obj *o, s32 a);
extern void func_80159B70(Obj *o);
extern void func_801725F4(Obj *o);
extern void func_80146CA0(Obj *o);

void func_8015D104(Obj *o)
{
    func_80149FB0(o);
    func_8014FA04(o);

    if (func_8014E6A0(o) == 0) {
        ((Handler)D_8018730C[o->f0])(o);
        func_80147078(o, 0);
        func_80159B70(o);
        return;
    }

    func_801725F4(o);
    o->f234 = (s32)0xFF1D0000;
    o->f20->f10 = 0x600;
    o->f244 = o->f20->f10;
    func_80146CA0(o);
}

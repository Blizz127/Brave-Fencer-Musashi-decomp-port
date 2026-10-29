/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_80148038(o,0x10000); func_80147460(o); func_80147AD4(o,0,0,0x1000);
 * func_801473EC(o); if o->fB8&0x8000, dispatches D_8018730C[o->f0](o) (discarded) then
 * func_80147078(o,0); func_80159B70(o) and returns; otherwise calls func_80161418(o) and,
 * when it returns 0, runs func_801725A4(o). Same Obj layout as the matched siblings
 * func_8015F9A4/func_8015DBD4/func_8015FE70. void, no caller found in any region. */
#include "psx_types.h"

typedef struct {
    u16 f0;  /* 0x0 */
    u8 pad2[0xB8 - 0x2];
    u16 fB8; /* 0xB8 */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_8018730C[];
extern void func_80148038(u8 *o, s32 d);
extern void func_80147460(Obj *o);
extern void func_80147AD4(void *o, s32 a, s32 b, s32 c);
extern void func_801473EC(u8 *o);
extern void func_80147078(Obj *o, s32 a);
extern void func_80159B70(Obj *o);
extern s32 func_80161418(Obj *o);
extern void func_801725A4(Obj *o);

void func_8015EEE0(Obj *o)
{
    func_80148038((u8 *)o, 0x10000);
    func_80147460(o);
    func_80147AD4(o, 0, 0, 0x1000);
    func_801473EC((u8 *)o);

    if (o->fB8 & 0x8000) {
        ((Handler)D_8018730C[o->f0])(o);
        func_80147078(o, 0);
        func_80159B70(o);
        return;
    }

    if (func_80161418(o) == 0) {
        func_801725A4(o);
    }
}

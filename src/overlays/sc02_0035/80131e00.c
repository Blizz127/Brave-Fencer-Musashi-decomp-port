/* Owner re-derivation from retail assembly (clean input set). */
/* Stores a1 to fB0, then calls the handler selected by a1 from table D_801849A0, passing the
 * object; returns the handler's v0 (many callers tail-forward it further). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xB0];
    u32 fB0;
} Obj;

extern u32 D_801849A0[];

s32 func_80131E00(Obj *o, s32 a1)
{
    o->fB0 = a1;
    return ((s32 (*)(Obj *))D_801849A0[a1])(o);
}

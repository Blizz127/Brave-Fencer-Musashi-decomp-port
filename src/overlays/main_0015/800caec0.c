/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Starts animation D_800CBAC0, applies effect 0x15 and advances the step. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x197];
    u8 step;            /* 0x197 */
} Obj;

extern u8 D_800CBAC0[];
extern void func_80154274(void *o, u32 v);
extern void func_80154A74(void *o, s32 i);

void func_800CAEC0(Obj *o)
{
    func_80154274(o, (u32)D_800CBAC0);
    func_80154A74(o, 0x15);
    o->step++;
}

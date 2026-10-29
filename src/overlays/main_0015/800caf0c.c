/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* On a settled state 0x8000 picks animation D_800CBB00 (bit 0x20 set) or D_800CBAE0 and advances the step by 1 or 2. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xB8];
    u16 mode;           /* 0xB8 */
    u8 padBA[0x197 - 0xBA];
    u8 step;            /* 0x197 */
} Obj;

extern u8 D_800CBAE0[];
extern u8 D_800CBB00[];
extern s32 func_80161208(void *o);
extern u16 func_801487F4(void *o);
extern void func_80154274(void *o, u32 v);

void func_800CAF0C(Obj *o)
{
    if (func_80161208(o) == 0 && o->mode == 0x8000) {
        if (func_801487F4(o) & 0x20) {
            func_80154274(o, (u32)D_800CBB00);
            o->step += 1;
        } else {
            func_80154274(o, (u32)D_800CBAE0);
            o->step += 2;
        }
    }
}

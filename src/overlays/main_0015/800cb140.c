/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* On a settled state 0x8000 runs func_801599A4 then func_80159B3C. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xB8];
    u16 mode;           /* 0xB8 */
} Obj;

extern s32 func_80161208(void *o);
extern s32 func_801599A4(void *o);
extern void func_80159B3C(void *o);

void func_800CB140(Obj *o)
{
    if (func_80161208(o) == 0 && o->mode == 0x8000) {
        func_801599A4(o);
        func_80159B3C(o);
    }
}

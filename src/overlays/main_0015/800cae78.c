/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* When func_80161104 reports ready, runs func_800CAEC0 and advances the step once more. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x197];
    u8 step;            /* 0x197 */
} Obj;

extern s32 func_80161104(void *obj);
extern void func_800CAEC0(Obj *o);

void func_800CAE78(Obj *o)
{
    if (func_80161104(o) != 0) {
        func_800CAEC0(o);
        o->step++;
    }
}

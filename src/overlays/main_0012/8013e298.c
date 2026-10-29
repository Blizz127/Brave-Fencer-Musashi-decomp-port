/* Owner re-derivation from retail assembly (clean input set). */
/* True when |obj->f4 - D_80126CB0| lies in [0x2C1, 0x300). */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    s16 f4;
} Obj8013E298;

extern s16 D_80126CB0;

s32 func_8013E298(Obj8013E298 *obj)
{
    s32 d;

    d = obj->f4 - D_80126CB0;
    if (d < 0) {
        d = -d;
    }
    return (u32)(d - 0x2C1) < 0x3F;
}

/* Owner re-derivation from retail assembly (clean input set). */
/* True when the id halfword at +6 is 0x31, 0x32, 0x33 or 0x27B, or when func_800132BC(D_80126CAC, o) lies strictly between D_801274D8 and D_801274DC. */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u16 f6;
} Obj;

extern u8 D_80126CAC[];
extern s32 D_801274D8;
extern s32 D_801274DC;
extern s32 func_800132BC(void *a, void *b);

s32 func_80184B94(Obj *o)
{
    s32 t = o->f6;
    s32 d;

    if ((u32)(t - 0x31) < 2) {
        return 1;
    }
    if ((u16)t == 0x33 || (u16)t == 0x27B) {
        return 1;
    }
    d = func_800132BC(D_80126CAC, o);
    if (D_801274D8 < d && d < D_801274DC) {
        return 1;
    }
    return 0;
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Calls the optional hook D_801274D0(pos); else 1 when D_801274D8 < dist2(D_80126CAC, pos) < D_801274DC. */
#include "psx_types.h"

typedef s32 (*Hook8013E448)(s16 *pos);

extern u32 D_801274D0; /* guest pointer: optional hook */
extern s32 D_801274D8;
extern s32 D_801274DC;
extern s16 D_80126CAC[];

extern s32 func_800132BC(s16 *a, s16 *b);

s32 func_8013E448(s16 *pos)
{
    Hook8013E448 hook;
    s32 t;

    hook = (Hook8013E448)D_801274D0;
    if (hook == 0) {
        t = func_800132BC(D_80126CAC, pos);
        if (t > D_801274D8 && t < D_801274DC) {
            return 1;
        }
        return 0;
    }
    return hook(pos);
}

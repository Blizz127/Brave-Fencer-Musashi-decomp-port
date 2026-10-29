/* Owner re-derivation from retail assembly (clean input set). */
/* If D_80126B3E != 0, runs func_80171CC4(o, &D_80126B38). Then unless func_80172664(o) is
 * nonzero, runs func_80171A1C(o); func_80174650(o). No caller found in any available region. */
#include "psx_types.h"

extern s16 D_80126B3E;
extern s32 D_80126B38;
extern s32 func_80171CC4(void *o, s32 a);
extern u32 func_80172664(void *o);
extern void func_80171A1C(void *o);
extern void func_80174650();

void func_80173648(void *o)
{
    s16 *p = &D_80126B3E;

    if (*p != 0) {
        func_80171CC4(o, (s32)((u8 *)p - 6));
    }
    if (func_80172664(o) == 0) {
        func_80171A1C(o);
        func_80174650(o);
    }
}

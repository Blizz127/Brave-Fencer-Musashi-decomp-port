/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern u8 D_8011D030[];
extern u16 D_80126B5E;
extern u16 D_80126B62;
extern u16 D_80126B66;
extern void func_80146C3C(u8 *o);
extern s32 func_80146A6C(s32 a, u8 *o, s32 x, s16 y, s32 z, s32 c1, s32 c2);

void func_8016AA50(u8 *o, s32 c)
{
    s16 i;
    u8 *e;
    u16 t;

    if (c >= 0) {
        func_80146A6C(0x1D, o, *(s16 *)(o + 0x7C), *(s16 *)(o + 0x7E),
                      *(s16 *)(o + 0x80), 0, c);
    } else {
        for (i = 0, e = D_8011D030; i < 0x1E; i++, e += 0x58) {
            t = *(u16 *)e;
            if (t == 0 || t == 3 || t == 9) {
                func_80146C3C(e);
                func_80146A6C(0x1D, o, *(s16 *)&D_80126B5E, D_80126B62 - 0x20,
                              *(s16 *)&D_80126B66, 7, 0);
                break;
            }
        }
    }
}

/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 func_80171CC4(void *o, u16 *p);
extern s32 func_80171D1C(void *o, u16 *p, s32 a);
extern void func_80171928(void *o);
extern u8 D_8011F73E[];

void func_8016F95C(void *o) {
    u16 *p = ((u16 *)&D_8011F73E);
    u16 x = *p;
    s32 r;
    if (x == 0) {
        r = func_80171CC4(o, p - 3);
    } else {
        r = func_80171D1C(o, p - 3, x & 0xFF);
    }
    if (r != 0) {
        func_80171928(o);
    }
}

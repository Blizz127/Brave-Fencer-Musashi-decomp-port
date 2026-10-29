#include "psx_types.h"

extern u8 D_8011F7A8[];
extern u8 D_8011F7B1;
extern u16 D_8011F7B4;
extern u16 D_8011F7B6;
extern s16 D_80078EA6;
extern s16 D_8011F81E;
extern s32 func_80178004(u32 a, s16 x, s16 y);

void func_80176D00(s16 i) {
    u32 *w;
    u32 *p;
    u32 v;
    w = ((u32 *)&D_8011F7A8);
    if (D_8011F7B1 == 0) {
        p = w + i;
        v = p[8] + 0x14;
        *(u32 *)((u8 *)p + 0x38) = v;
        (*(u16 *)&D_8011F81E) = (*(u16 *)&D_80078EA6);
        p[16] = func_80178004(v, D_8011F7B4 - 0x17, D_8011F7B6 + 0x65) - 0x14;
    }
}

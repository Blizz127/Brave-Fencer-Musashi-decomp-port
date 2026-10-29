#include "psx_types.h"
typedef struct { u32 a; u32 b; } E8;
extern u32 D_801151E0[3];
extern u8 D_80126AF0[];
extern s16 D_80126AF6[][4];
extern u8 D_80128120[][8];
extern u8 D_80128138[][8];
extern u32 D_801150E0[];
extern u8 D_8011DAD8[][8];
extern u8 D_8011520C[];
extern u8 D_80126730[];
extern s16 D_801270A8[3];
extern void func_80156B74(void *o, s32 m, void *p);
extern s32 func_80157158(s32 a0, u16 a1, u16 a2, s32 a3, s32 b0, s32 b1, s32 b2, s32 b3, s32 b4, s32 b5, u16 b6, s32 b7, s32 b8);
void func_80156670(void *o, s32 a1, s32 a2, s32 a3, s32 a4, u16 a5) {
    u32 i;
    func_80156B74(o, a3, (u8 *)o + 0x1A9);
    for (i = 0; i < 3; i++) {
        D_801151E0[i] = 0;
        if (D_80126AF6[i][0] == 0) {
            D_801151E0[i] = func_80157158((s32)o, a1, a2, (s32)D_801270A8, (s32)&((E8 *)&D_80126AF0)[i], (s32)&D_80128120[i], (s32)&D_80128138[i], (s32)&D_801150E0[i], (s32)&D_8011DAD8[i], a4, a5, (s32)&D_8011520C[i], (s32)&D_80126730[i]);
        }
    }
}

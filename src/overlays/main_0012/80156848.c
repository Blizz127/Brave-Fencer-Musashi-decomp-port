/* Owner re-derivation from retail assembly (clean input set). */
/* For each of the three slots: slot flag bit 0 starts a fixed effect, bits 1-2 start a flagged effect (and a follow-up when the slot byte is 0x24). */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 w;
} E8;

extern u32 D_801151E0[];
extern u8 D_80126730[];
extern E8 D_80126AF0[];
extern u8 D_80128120[][8];
extern u8 D_80128138[][8];
extern void func_80013E94(void *a, u32 *b);
extern s32 func_801469C8(u16 a, s32 b, s16 c, s16 d, s16 e, s32 f, s32 g, u32 lim);
extern s32 func_80146A6C(s32 a, void *o, s32 x, s16 y, s32 z, u32 c1, u32 c2);
extern void func_80156A1C(void *o, s32 i);

void func_80156848(u8 *o, u8 *r) {
    u32 i;
    u32 *fp;
    u32 *base;
    u32 w;
    for (i = 0; i < 3; i++) {
        base = D_801151E0;
        fp = base + i;
        if (*fp & 1) {
            func_80013E94(D_80128120[i], &w);
            func_80146A6C(r[8], o, D_80126AF0[i].x, D_80126AF0[i].y, D_80126AF0[i].z, w, r[9]);
        } else if (*fp & 6) {
            func_80156A1C(o, i);
            func_80013E94(D_80128138[i], &w);
            if ((*fp & 6) == 4) {
                w |= 0x80000000;
            }
            func_80146A6C(6, o, D_80126AF0[i].x, D_80126AF0[i].y, D_80126AF0[i].z, w, 0);
            if (D_80126730[i] == 0x24) {
                func_801469C8(0x53, (s32)o, D_80126AF0[i].x, D_80126AF0[i].y, D_80126AF0[i].z, w, 0, 0x14);
            }
        }
    }
}

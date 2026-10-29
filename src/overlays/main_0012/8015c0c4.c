/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern s16 D_801152B0;
extern s16 D_801152B4;
extern void func_80154150(void *o, s32 a);
extern void func_801553A8(void *o);
extern void func_80146CA0(void *o);
void func_8015C0C4(char *o) {
    *(s32 *)(o + 0x234) = D_801152B0 << 6;
    *(s32 *)(o + 0x238) = 0x80000;
    *(s32 *)(o + 0x23C) = D_801152B4 << 6;
    func_80154150(o, 0x11);
    func_801553A8(o);
    func_80146CA0(o);
}

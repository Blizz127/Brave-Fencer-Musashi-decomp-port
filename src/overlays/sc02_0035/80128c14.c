/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s16 D_800B9A08;
extern void func_80010AE0(s32 a);

void func_80128C14(void) {
    s32 a;
    switch (D_800B9A08) {
    case 0x3023:
        a = 0x4E0;
        break;
    case 0x3067:
        a = 0x480;
        break;
    case 0x3081:
        a = 0x420;
        break;
    case 0x3093:
        a = 0xFFB;
        break;
    case 0x30A5:
        a = 0xFFB;
        break;
    default:
        a = 0x3E0;
        break;
    }
    func_80010AE0(a);
}

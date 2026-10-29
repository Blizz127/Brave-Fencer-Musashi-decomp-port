/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern s32 func_80131A34(u8 *o, s32 a);
extern s32 func_8012BCCC(u8 *o);
extern s32 func_80131AC8(u8 *o, s32 a);
extern s32 func_80131CA8(u8 *o, s32 a);
extern void func_80131E00(u8 *o, s32 a);

void func_801300F4(u8 *o)
{
    s32 r;

    r = 0;
    o[0xC1] = 0xD;
    if ((*(u32 *)(o + 0xB4) & 0x2000) != 0) {
        *(u16 *)(o + 0x5C) = 0;
        *(s32 *)(o + 0x1C) = 0x1E;
        *(u16 *)(o + 0x98) = 0;
        *(u16 *)(*(u8 **)(o + 0x20) + 0x10) = 0;
        if (func_80131A34(o, 0x27) != 0) {
            o[0xC3] = 1;
        } else {
            o[0xC3] = 0;
        }
        if (func_8012BCCC(o) <= 0x8FFF) {
            if (func_80131AC8(o, 0x30) == 0) {
                func_80131AC8(o, 0x31);
            }
            r = 1;
        } else {
            if (func_80131AC8(o, 0x31) == 0) {
                func_80131AC8(o, 0x30);
            }
            r = 1;
        }
    }
    if (func_80131CA8(o, 0x25) != 0) {
        r = 1;
    }
    if (r == 0) {
        func_80131E00(o, 6);
    }
}

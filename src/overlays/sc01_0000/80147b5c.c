/* Fresh isolated derivation from sc01_0000 retail instructions. */
#include "psx_types.h"
extern s32 D_80127090;
extern s32 D_80127094;
extern s32 D_80127098;
void func_80147B5C(u8 *o, s32 *p)
{
    if (*(u32 *)(o + 0x44) & 0x00400000) {
        D_80127090 = *(s32 *)(o + 0x24);
    } else {
        D_80127090 = p[0];
    }
    if (*(u32 *)(o + 0x44) & 0x00800000) {
        D_80127094 = *(s32 *)(o + 0x28);
    } else {
        D_80127094 = p[1];
    }
    if (*(u32 *)(o + 0x44) & 0x01000000) {
        D_80127098 = *(s32 *)(o + 0x2C);
    } else {
        D_80127098 = p[2];
    }
    *(s32 *)(o + 0x10) = D_80127090;
    *(s32 *)(o + 0x14) = D_80127094;
    *(s32 *)(o + 0x18) = D_80127098;
    D_80127090 = 0;
    D_80127094 = 0;
    D_80127098 = 0;
}

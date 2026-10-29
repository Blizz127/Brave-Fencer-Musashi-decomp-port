#include "psx_types.h"

extern u8 D_800B9AAC[];
extern u8 D_800B9AAE[];
extern u8 D_800B9AB0[];
extern u8 D_800B9AB2[];
extern u8 D_800B9AB4[];
extern u8 D_800B9AB6[];
extern u8 D_800B9AB8[];
extern u8 D_800B9ABA[];

void func_80129398(void) {
    s32 i;
    s32 off;
    for (i = 0; i < 2; i++) {
        off = i * 0x54;
        *(u16 *)(D_800B9AAC + off) = 0x7FFF;
        *(u16 *)(D_800B9AAE + off) = 0;
        *(u16 *)(D_800B9AB0 + off) = 0xFFFF;
        *(u16 *)(D_800B9AB2 + off) = 0;
        *(u16 *)(D_800B9AB4 + off) = 0x140;
        *(u16 *)(D_800B9AB6 + off) = 0xEE;
        *(u16 *)(D_800B9AB8 + off) = 0;
        *(u16 *)(D_800B9ABA + off) = 0;
    }
}

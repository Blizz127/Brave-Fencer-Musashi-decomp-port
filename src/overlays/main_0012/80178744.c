#include "psx_types.h"

void func_80178744(u8 *o, s32 scale) {
    u32 c = *(u32 *)(o + 0x6C);
    u32 m;
    u16 v;
    if (c != 0) {
        func_80178744((u8 *)c, scale);
    }
    v = scale;
    m = *(u32 *)(o + 0x20);
    *(u16 *)(m + 0x1C) = v;
    *(u16 *)(m + 0x1A) = v;
    *(u16 *)(m + 0x18) = v;
    if (scale == 0x1000) {
        *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) = *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) & 0xFFEF;
    } else {
        *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) = *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) | 0x10;
    }
}

void func_80178744c(u8 *o, s32 scale) {
    u32 c = *(u32 *)(o + 0x6C);
    u32 m;
    u32 v;
    if (c != 0) {
        func_80178744c((u8 *)c, scale);
    }
    v = scale;
    m = *(u32 *)(o + 0x20);
    *(u16 *)(m + 0x1C) = v;
    *(u16 *)(m + 0x1A) = v;
    *(u16 *)(m + 0x18) = v;
    if (scale == 0x1000) {
        *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) = *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) & 0xFFEF;
    } else {
        *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) = *(u16 *)(*(u32 *)(o + 0x20) + 0x2C) | 0x10;
    }
}

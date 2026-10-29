/* func_80128DB4 - owner re-derivation from retail assembly (clean input set).
 * Steps an animation cursor (u16 index at +4, s16 count at +6) over 8-byte
 * entries; on a new entry calls func_80018450(a, w) or func_800183E0(w).
 * Returns 1 when finished/stopped, else 0 (v0 set on every path). */
#include "psx_types.h"
extern void func_80018450(s32 a, u32 b);
extern void func_800183E0(u32 a);

s32 func_80128DB4(s32 a, u8 *p)
{
    u8 *t;
    s16 c;
    s16 i;
    u32 e;

    c = *(s16 *)(p + 6);
    t = *(u8 **)p;
    if (c == 0) {
        return 1;
    }
    c = c - 1;
    *(s16 *)(p + 6) = c;
    if (c > 0) {
        return 0;
    }
    i = *(s16 *)(p + 4);
    e = *(u32 *)(t + i * 8 + 4);
    if ((e & 0xC0) == 0xC0) {
        *(s16 *)(p + 6) = 0;
        return 1;
    }
    if ((e & 0x80) != 0) {
        *(s16 *)(p + 4) = 0;
    } else {
        *(s16 *)(p + 4) = i + 1;
    }
    *(s16 *)(p + 6) = *(u32 *)(t + *(s16 *)(p + 4) * 8 + 4) & 0x3F;
    if (a != 0) {
        func_80018450(a, *(u32 *)(t + *(s16 *)(p + 4) * 8));
    } else {
        func_800183E0(*(u32 *)(t + *(s16 *)(p + 4) * 8));
    }
    return 0;
}

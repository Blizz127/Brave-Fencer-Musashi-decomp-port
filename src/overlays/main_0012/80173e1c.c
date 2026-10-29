/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern s32 D_80127508;
extern u8 D_800D5DC0[];
extern void func_801484B0(u8 *a, u8 *b);
extern void func_80149350(u8 *o, u8 *a, u8 *b);
extern void func_80015954(u8 *a, u32 b);
extern s32 func_801399F0(u32 h);
extern void func_80139914(u32 h);
extern void func_80154274(u8 *o, u8 *p);
extern void func_80171A1C(u8 *o);

void func_80173E1C(u8 *o)
{
    u8 v[16];

    if (*(u32 *)&D_80127508 != 0) {
        func_801484B0(v, v + 8);
        func_80149350(o, v, v);
        func_80015954(v, *(u32 *)&D_80127508 + 4);
    }
    if (*(u32 *)(o + 0x198) != 0) {
        if (func_801399F0(*(u32 *)(o + 0x198)) != 0) {
            func_80139914(*(u32 *)(o + 0x198));
            *(u32 *)(o + 0x198) = 0;
            func_80154274(o, D_800D5DC0);
            func_80171A1C(o);
        }
    } else {
        s32 n = *(s32 *)(o + 0x200) - 1;
        *(s32 *)(o + 0x200) = n;
        if (n == -1) {
            func_80154274(o, D_800D5DC0);
            func_80171A1C(o);
        }
    }
}

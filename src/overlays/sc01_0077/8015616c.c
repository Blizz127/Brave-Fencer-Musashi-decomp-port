#include "psx_types.h"

extern u16 *func_801563EC(u32 index);
extern s32 func_80029B4C(s16 base, s16 modifier);
extern s32 func_80029BC8(s16 base, s16 modifier);
extern s32 func_80029C44(s16 base, s16 modifier);
extern s32 func_8005C49C(void);
extern s32 func_80029178(u32 flag);
extern s32 func_8015640C(void *actor, u16 strength);
extern s16 D_8012693A, D_801152B8;

u16 func_8015616C(void *actor, u16 index)
{
    u16 *row = func_801563EC(index);
    s32 rank;
    s32 flags = row[0];
    u32 strength;
    s32 spread = row[1];
    s32 other_base = row[2];
    s32 other_modifier = row[3];
    s32 effect = row[4];
    s32 type = row[5];
    s32 base = flags & 0xfff;
    if (flags & 0x4000) {
        s32 first = func_80029B4C(base, spread);
        strength = (u32)first + (u32)func_80029BC8(other_base, other_modifier);
    } else if (flags & 0x2000) {
        strength = func_80029C44(base, spread);
    } else {
        strength = base + func_8005C49C() % (spread + 1);
    }
    D_8012693A = (s16)effect;
    if ((u8)func_80029178(0x81))
        D_8012693A = (s16)((u16)D_8012693A * 2);
    rank = 0;
    if (type) {
        if ((u8)func_80029178(0x1a))
            rank = 5;
        else if ((u8)func_80029178(0x19))
            rank = 4;
        else if ((u8)func_80029178(0x18))
            rank = 3;
        else if ((u8)func_80029178(0x17))
            rank = 2;
        else if ((u8)func_80029178(0x16))
            rank = 1;
    }
    D_801152B8 = (s16)(type + rank);
    if (flags & 0x8000)
        strength = func_8015640C(actor, strength);
    return strength;
}

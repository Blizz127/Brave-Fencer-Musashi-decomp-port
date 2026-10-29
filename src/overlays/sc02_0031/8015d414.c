/* Owner re-derivation from retail assembly (clean input set). */
/* Sets o->0x1BD=8, o->0x38=&D_80186620, zeroes the four u16 fields at o+0x138/0x13A/0x13C/0x13E,
 * bumps o->0xA (u16) by 0x48, clears (o->0x20)->0x10, then func_801553C0(o); func_80154A74(o,0x11). */
#include "psx_types.h"

extern s32 D_80186620;
extern void func_801553C0(u8 *o);
extern void func_80154A74(u8 *o, s32 a1);

void func_8015D414(u8 *o)
{
    u16 f0A;
    u8 *v1;

    *(u8 *)(o + 0x1BD) = 8;
    *(s32 *)(o + 0x38) = (s32)&D_80186620;
    f0A = *(u16 *)(o + 0xA);
    v1 = *(u8 **)(o + 0x20);
    *(u16 *)(o + 0x138) = 0;
    *(u16 *)(o + 0x13A) = 0;
    *(u16 *)(o + 0x13C) = 0;
    *(u16 *)(o + 0x13E) = 0;
    *(u16 *)(o + 0xA) = f0A + 0x48;
    *(u16 *)(v1 + 0x10) = 0;
    func_801553C0(o);
    func_80154A74(o, 0x11);
}

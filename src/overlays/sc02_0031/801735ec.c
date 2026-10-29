/* Owner re-derivation from retail assembly (clean input set). */
/* If D_80126B40 is nonzero, sets bit 0x40000 of o->0x44. Then func_801477E8(o, D_80126B32<<16)
 * and func_80171A1C(o). */
#include "psx_types.h"

extern s16 D_80126B40;
extern s16 D_80126B32;
extern void func_801477E8(u8 *o, s32 a);
extern void func_80171A1C(u8 *o);

void func_801735EC(u8 *o)
{
    if (D_80126B40 != 0) {
        *(s32 *)(o + 0x44) |= 0x40000;
    }
    func_801477E8(o, (s32)D_80126B32 << 16);
    func_80171A1C(o);
}

/* Owner re-derivation from retail assembly (clean input set). */
/* If D_80126B30 is nonzero, sets bit 0x10 of o->0xAC; else sets bit 0x80 of o->0xAC and
 * o->0xAA; then calls func_80171A1C(o). */
#include "psx_types.h"

extern s16 D_80126B30;
extern void func_80171A1C(u8 *o);

void func_80173980(u8 *o)
{
    if (D_80126B30 == 0) {
        *(u16 *)(o + 0xAC) |= 0x80;
        *(u16 *)(o + 0xAA) |= 0x80;
    } else {
        *(u16 *)(o + 0xAC) |= 0x10;
    }
    func_80171A1C(o);
}

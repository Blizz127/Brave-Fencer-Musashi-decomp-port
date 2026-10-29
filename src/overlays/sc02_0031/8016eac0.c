/* Owner re-derivation from retail assembly (clean input set). */
/* o->f4 += D_80188030[o->f2]; func_8016EC0C(o,0x80); o->f2++; once the (byte) counter reaches 6,
 * resets it to 0x20 and bumps o->f1. No caller found in any available region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[1];
    u8 f1;   /* 0x1 */
    u8 f2;   /* 0x2 */
    u8 pad3[1];
    u16 f4;  /* 0x4 */
} Obj;

extern u16 D_80188030[];
extern void func_8016EC0C(Obj *o, s32 a);

void func_8016EAC0(Obj *o)
{
    o->f4 = o->f4 + D_80188030[o->f2];
    func_8016EC0C(o, 0x80);

    o->f2 = o->f2 + 1;
    if ((u8)o->f2 >= 6) {
        o->f2 = 0x20;
        o->f1 = o->f1 + 1;
    }
}

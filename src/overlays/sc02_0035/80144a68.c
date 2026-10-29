/* Owner re-derivation from retail assembly (clean input set). */
/* Sets o->f0 = 0xC6 then forwards to func_8012AD44(o, 0). */
#include "psx_types.h"

extern void func_8012AD44(u8 *o, u16 a);

void func_80144A68(u8 *o)
{
    *(u16 *)(o + 0) = 0xC6;
    func_8012AD44(o, 0);
}

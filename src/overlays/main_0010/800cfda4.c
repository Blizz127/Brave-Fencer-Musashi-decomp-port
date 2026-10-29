/* Owner re-derivation from retail assembly (clean input set). */
/* Passes the 12-bit field of the current mode's entry in the word table D_800D34AC to func_800191BC. */
#include "psx_types.h"

extern void func_800191BC(s32 a);
extern s16 D_800B9A08;
extern u32 D_800D34AC[];

void func_800CFDA4(void)
{
    func_800191BC(D_800D34AC[D_800B9A08 & 0xFFFF0FFF] & 0xFFF);
}

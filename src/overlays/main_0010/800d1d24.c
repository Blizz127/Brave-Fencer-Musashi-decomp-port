/* Owner re-derivation from retail assembly (clean input set). */
/* Latches the pending mode word D_80078E50 as the current mode D_800B9A08 (remembering the old D_800B9A0A in D_800B9A0C and keeping only the top nibble in D_800B9A0A), sets the flag D_800B9A11, then runs the reset calls func_8001C0C8, func_80015310, func_80129258 and func_801378F0. */
#include "psx_types.h"

extern void func_8001C0C8(void);
extern void func_80015310(void);
extern void func_80129258(void);
extern void func_801378F0(void);
extern u16 D_80078E50;
extern s16 D_800B9A08;
extern u16 D_800B9A0A;
extern u16 D_800B9A0C;
extern u8 D_800B9A11;

void func_800D1D24(void)
{
    u16 v;
    u16 old;

    v = D_80078E50;
    old = D_800B9A0A;
    D_800B9A11 = 1;
    D_800B9A08 = v;
    D_800B9A0C = old;
    D_800B9A0A = v & 0xF000;
    func_8001C0C8();
    func_80015310();
    func_80129258();
    func_801378F0();
}

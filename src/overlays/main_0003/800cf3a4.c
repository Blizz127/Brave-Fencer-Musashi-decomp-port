/* Owner re-derivation from retail assembly (clean input set). */
/* Passes the table D_800D52BC to func_800183E0, sets the half-word D_800B99E6 to 1 and copies the word D_800EC680 into
 * the half-word D_800B99E8. */
#include "psx_types.h"

extern u8 D_800D52BC[];
extern s32 D_800EC680;
extern u16 D_800B99E6;
extern u16 D_800B99E8;
extern void func_800183E0(u32 a);

void func_800CF3A4(void)
{
    func_800183E0((u32)D_800D52BC);
    D_800B99E6 = 1;
    D_800B99E8 = D_800EC680;
}

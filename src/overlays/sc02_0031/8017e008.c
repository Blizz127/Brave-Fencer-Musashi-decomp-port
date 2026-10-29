/* Owner re-derivation from retail assembly (clean input set). */
/* Adds 8 to D_801888FC (s16 arithmetic), stores the new value into D_801888FC/FA/F8, and
 * if the result is >= 0x101 calls func_80146CA0(void). */
#include "psx_types.h"

extern u16 D_801888FC;
extern u16 D_801888FA;
extern u16 D_801888F8;
extern void func_80146CA0(void);

void func_8017E008(void)
{
    s16 v = (s16)(D_801888FC + 8);
    D_801888FC = (u16)v;
    D_801888FA = (u16)v;
    D_801888F8 = (u16)v;
    if (v >= 0x101) {
        func_80146CA0();
    }
}

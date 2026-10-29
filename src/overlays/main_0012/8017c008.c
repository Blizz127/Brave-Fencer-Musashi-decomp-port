/* Owner re-derivation from retail assembly (clean input set). */
/* Starts a mode: sets the byte flags D_800B9A15 = 1 and D_800B9A13 = 5, D_80115110 = 4, stores the two arguments (a into D_80115126, b into D_801857F0 and D_801857EC), clears D_80115112, installs func_8017C080 in D_8011DB24 and calls func_8002D4C8(0x1E, 0). */
#include "psx_types.h"

extern u8 D_800B9A15;
extern u8 D_800B9A13;
extern u16 D_80115110;
extern u16 D_80115126;
extern u16 D_801857F0;
extern u16 D_801857EC;
extern u16 D_80115112;
extern u32 D_8011DB24;
extern void func_8017C080(void);
extern void func_8002D4C8(u16 a, u16 b);

void func_8017C008(u16 a, u16 b)
{
    D_800B9A15 = 1;
    D_800B9A13 = 5;
    D_80115110 = 4;
    D_80115126 = a;
    D_801857F0 = b;
    D_801857EC = b;
    D_80115112 = 0;
    D_8011DB24 = (u32)func_8017C080;
    func_8002D4C8(0x1E, 0);
}

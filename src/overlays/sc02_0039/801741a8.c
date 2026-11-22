/* Run the two refills and clear service before restoring the paired fixed-point fields. */
#include "mine_stat_event.h"

extern void func_8014BDC8(void);
extern void func_8014BB0C(void);
extern void func_8014B2A8(void);
extern u32 D_80078ED8;
extern u32 D_80078EA4;

void func_801741A8(void *actor)
{
    (void)actor;
    func_8014BDC8();
    func_8014BB0C();
    func_8014B2A8();
    D_80078ED8 = 0xa00000;
    D_80078EA4 = 0xa00000;
}

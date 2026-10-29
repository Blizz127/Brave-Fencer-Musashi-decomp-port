/* Owner re-derivation from retail assembly (clean input set). */
/* Registers the active pointer into D_801BD680, clears the two flag bytes that follow it, then
 * when func_80019198 reports failure (zero) runs func_80019064 on the D_80062BC0 message/table. */
#include "psx_types.h"

extern void *D_801BD680;
extern u8 D_801BD684;
extern u8 D_801BD685;
extern s32 func_80019198(void);
extern void func_80019064(void *p);
extern u8 D_80062BC0[];

void func_8013C9C4(void *p)
{
    D_801BD680 = p;
    D_801BD684 = 0;
    D_801BD685 = 0;
    if (func_80019198() == 0) {
        func_80019064(D_80062BC0);
    }
}

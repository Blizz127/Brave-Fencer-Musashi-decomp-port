/* Owner re-derivation from retail assembly (clean input set). */
/* Resets three globals, clears o+0xA8, then runs a fixed sequence of init calls on o. */
#include "psx_types.h"

extern s32 D_801B6440, D_801B644C, D_801B6450;
extern void func_80147084(u8 *o);
extern void func_80147098(u8 *o);
extern void func_801470AC(u8 *o);
extern void func_80147078(u8 *o, u16 a);
extern void func_80146C98(u8 *o, s32 a1);
extern void func_80158880(u8 *o);

void func_8015879C(u8 *o)
{
    D_801B6440 = 0;
    D_801B644C = 1;
    D_801B6450 = 0;
    *(u8 *)(o + 0xA8) = 0;
    func_80147084(o);
    func_80147098(o);
    func_801470AC(o);
    func_80147078(o, 0x1A);
    func_80146C98(o, 1);
    func_80158880(o);
}

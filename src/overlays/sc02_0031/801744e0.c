/* Owner re-derivation from retail assembly (clean input set). */
/* Stores D_80127508 via func_800D1724, then runs func_80171A1C(o) and func_80174650(o). */
#include "psx_types.h"

extern s32 D_80127508;
extern void func_800D1724(s32 a);
extern void func_80171A1C(u8 *o);
extern void func_80174650(void *o);

void func_801744E0(u8 *o)
{
    func_800D1724(D_80127508);
    func_80171A1C(o);
    func_80174650(o);
}

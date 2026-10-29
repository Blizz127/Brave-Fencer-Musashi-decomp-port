/* Owner re-derivation from retail assembly (clean input set). */
/* Sets rec2+0x27 to 0x82, then runs func_800183E0 on &D_801ABB28[(D_80189008[D_800B99DA&0x1F]) * 0x10]. */
#include "psx_types.h"

extern u16 D_800B99DA;
extern u8 D_80189008[];
extern u8 D_801ABB28[];
extern s32 func_800183E0(void *a);

void func_80183098(u8 *o)
{
    u8 *rec2 = *(u8 **)(o + 0xD0);
    u32 idx;
    u8 kind;

    rec2[0x27] = 0x82;
    idx = D_800B99DA & 0x1F;
    kind = D_80189008[idx];
    func_800183E0(D_801ABB28 + (kind << 4));
}

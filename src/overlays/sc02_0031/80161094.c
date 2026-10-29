/* Owner re-derivation from retail assembly (clean input set). */
/* If func_801399F0(o->f198) is nonzero: re-reads o->f198, calls func_80139914 on it and
 * clears the field. Either way, sets D_801270C0=1, func_80129248(1), D_800B9A17=1 (retail
 * stores the literal 1 at every path, so the intermediate always-1 result register is
 * replaced here by the constant directly). */
#include "psx_types.h"

extern s32 func_801399F0(u32 a);
extern void func_80139914(u32 a);
extern s16 D_801270C0;
extern void func_80129248(s32 a);
extern u8 D_800B9A17;

void func_80161094(u8 *o)
{
    u32 p = *(u32 *)(o + 0x198);
    if (func_801399F0(p) != 0) {
        p = *(u32 *)(o + 0x198);
        func_80139914(p);
        *(u32 *)(o + 0x198) = 0;
    }
    D_801270C0 = 1;
    func_80129248(1);
    D_800B9A17 = 1;
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: while D_80127084 is set, runs func_80011CFC when the low 7 bits of D_80078EC0 are 0 or func_800D0488 accepts them. */
#include "psx_types.h"

extern s32 func_800D0488(s32 a);
extern void func_80011CFC(void);
extern s32 D_80127084;
extern u8 D_80078EC0;

void func_800CF5D4(void)
{
    s32 v;

    if (D_80127084 != 0) {
        v = D_80078EC0 & 0x7F;
        if (v == 0 || func_800D0488(v) != 0) {
            func_80011CFC();
        }
    }
}

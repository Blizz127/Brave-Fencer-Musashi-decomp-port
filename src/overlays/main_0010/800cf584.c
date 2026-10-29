/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: while D_80127084 is set, runs func_80011CFC when the byte D_80078EC1 is 0 or func_800D02D0 accepts it. */
#include "psx_types.h"

extern s32 func_800D02D0(s32 a);
extern void func_80011CFC(void);
extern s32 D_80127084;
extern u8 D_80078EC1;

void func_800CF584(void)
{
    u8 b;

    if (D_80127084 != 0) {
        b = D_80078EC1;
        if (b == 0 || func_800D02D0(b) != 0) {
            func_80011CFC();
        }
    }
}

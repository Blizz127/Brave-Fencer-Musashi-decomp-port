/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: when func_800CFE60 reports a hit, runs func_800CFDA4, loads the mode word D_800B9A08 from D_80114E94 and calls func_80011CFC. */
#include "psx_types.h"

extern void func_800191A8(void);
extern s32 func_800CFE60(void);
extern void func_800CFDA4(void);
extern void func_80011CFC(void);
extern s16 D_800B9A08;
extern s32 D_80114E94;

void func_800CF714(void)
{
    func_800191A8();
    if (func_800CFE60() != 0) {
        func_800CFDA4();
        D_800B9A08 = D_80114E94;
        func_80011CFC();
    }
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Scene setup: runs func_80029344, clears D_80127084 and D_80114E78 (setting D_80127084 when the half-word
 * D_800B9A00 is nonzero), then runs func_8001B324, func_8002AEF8, func_80029254, func_800CFBBC, func_80011DCC,
 * func_80010AE0(0x3E0) and func_80011C10. */
#include "psx_types.h"

extern s16 D_800B9A00;
extern s32 D_80127084;
extern s32 D_80114E78;
extern void func_80029344(void);
extern void func_8001B324(void);
extern void func_8002AEF8(void);
extern void func_80029254(void);
extern void func_800CFBBC(void);
extern void func_80011DCC(void);
extern void func_80010AE0(s32 a);
extern s32 func_80011C10(void);

/* Returns retail's leftover v0: func_80011C10's result. */
s32 func_800CEE84(void)
{
    func_80029344();
    D_80127084 = 0;
    D_80114E78 = 0;
    if (D_800B9A00 != 0) {
        D_80127084 = 1;
    }
    func_8001B324();
    func_8002AEF8();
    func_80029254();
    func_800CFBBC();
    func_80011DCC();
    func_80010AE0(0x3E0);
    return func_80011C10();
}

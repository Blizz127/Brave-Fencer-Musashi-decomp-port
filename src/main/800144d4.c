/* Owner re-derivation from retail assembly (clean input set). */
/* Resets a block of globals to initial values and calls two setup routines. */
#include "psx_types.h"

extern s32 D_80126950;
extern s32 D_80126984;
extern s32 D_80126988;
extern s32 D_8012698C;
extern s32 D_80126990;
extern s32 D_80126994;
extern s32 D_80126998;
extern s32 D_8012699C;
extern s32 D_801269A0;
extern void func_80053308(s32);
extern void func_80053F6C(s32 *);

void func_800144D4(void) {
    D_80126950 = 0x12C;
    func_80053308(0x12C);
    D_80126984 = 0;
    D_80126988 = 0;
    D_8012698C = -0x12C;
    D_80126990 = 0;
    D_80126994 = 0;
    D_80126998 = 0;
    D_8012699C = 0;
    D_801269A0 = 0;
    func_80053F6C(&D_80126984);
}

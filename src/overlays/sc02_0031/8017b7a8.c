/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 D_80114F30;
extern s32 D_80114F34;
extern s32 D_80114F38;
extern s32 D_80114F24;
extern s32 D_80114F28;
extern s32 D_80114F2C;

extern u8 D_8012694C;
extern u16 D_801B7504;
extern u16 D_801B757C;
extern u16 D_801B757E;
extern u16 D_801B7580;
extern u16 D_801B7574;
extern u16 D_801B7576;
extern u16 D_801B7578;

void func_8017B7A8(void) {
    D_8012694C = 1;
    D_801B7504 = 0;
    D_801B757C = D_80114F30;
    D_801B757E = D_80114F34;
    D_801B7580 = D_80114F38;
    D_801B7574 = D_80114F24;
    D_801B7576 = D_80114F28;
    D_801B7578 = D_80114F2C;
}

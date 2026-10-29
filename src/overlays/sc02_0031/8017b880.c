/* Owner re-derivation from retail assembly (clean input set). */
/* Copies six s16 globals (sign-extended to s32) into six s32 destination globals. */
#include "psx_types.h"

extern s16 D_801B7594, D_801B7596, D_801B7598, D_801B759C, D_801B759E, D_801B75A0;
extern s32 D_80114F30, D_80114F34, D_80114F38, D_80114F24, D_80114F28, D_80114F2C;

void func_8017B880(void)
{
    D_80114F30 = D_801B7594;
    D_80114F34 = D_801B7596;
    D_80114F38 = D_801B7598;
    D_80114F24 = D_801B759C;
    D_80114F28 = D_801B759E;
    D_80114F2C = D_801B75A0;
}

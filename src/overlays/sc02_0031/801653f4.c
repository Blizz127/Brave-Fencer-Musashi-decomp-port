/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u8 D_80078EC1;
extern u8 D_80078EC5;
extern s16 D_800B9A08;
extern s32 func_80165624(void);

void func_801653F4(void) {
    u8 v0 = D_80078EC1;
    s32 s0 = D_800B9A08;

    D_80078EC5 = 0;
    v0 &= 0x7F;
    D_80078EC1 = v0;

    if (func_80165624() != 0) {
        D_80078EC5 = 0x80;
        return;
    }

    if (D_80078EC1 != 0x1A) {
        return;
    }

    if ((u16)(s0 - 0x305C) < 2) return;
    if ((u16)(s0 - 0x305E) < 2) return;
    if ((u16)(s0 - 0x3060) < 2) return;
    if ((u16)(s0 - 0x3062) < 2) return;
    if ((u16)(s0 - 0x3064) < 2) return;
    if (s0 == 0x3066) return;

    D_80078EC5 = 0x80;
}

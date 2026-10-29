/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements a counter halfword and clears seven neighbouring halfwords in a large global block. */
#include "psx_types.h"

typedef struct Big {
    u8 pad0[0xA3C0];
    u16 cA3C0;
    u16 cA3C2;
    u8 padA3C4[2];
    u16 cA3C6;
    u16 cA3C8;
    u16 cA3CA;
    u16 cA3CC;
    u16 cA3CE;
    u16 cA3D0;
} Big;

extern Big D_800AF630;

void func_80011ADC(void) {
    register Big *p = &D_800AF630;

    p->cA3C0--;
    p->cA3C6 = 0;
    p->cA3CC = 0;
    p->cA3CA = 0;
    p->cA3D0 = 0;
    p->cA3C2 = 0;
    p->cA3C8 = 0;
    p->cA3CE = 0;
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Advances a halfword counter and clears seven neighbouring halfword fields. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_80011A3C(void) {
    register u8 *p = D_800AF630;
    (*(u16 *)(p + 0xA3C0))++;
    *(u16 *)(p + 0xA3C6) = 0;
    *(u16 *)(p + 0xA3CC) = 0;
    *(u16 *)(p + 0xA3CA) = 0;
    *(u16 *)(p + 0xA3D0) = 0;
    *(u16 *)(p + 0xA3C2) = 0;
    *(u16 *)(p + 0xA3C8) = 0;
    *(u16 *)(p + 0xA3CE) = 0;
}

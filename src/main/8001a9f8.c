/* Owner re-derivation from retail assembly (clean input set). */
/* Links the two fixed full-screen packets (length 5) into slot 1 of the current ordering table. */
#include "psx_types.h"

extern u32 D_80063108[];
extern u32 D_80063120[];
extern u32 D_800A6610[][4096];
extern u16 D_800B9A02;

void func_8001A9F8(void) {
    u32 *ot;

    ot = D_800A6610[D_800B9A02];
    D_80063108[0] = (ot[1] & 0xFFFFFF) | 0x05000000;
    ot[1] = (ot[1] & 0xFF000000) | ((u32)D_80063108 & 0xFFFFFF);
    D_80063120[0] = (ot[1] & 0xFFFFFF) | 0x05000000;
    ot[1] = (ot[1] & 0xFF000000) | ((u32)D_80063120 & 0xFFFFFF);
}

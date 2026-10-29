/* Independently re-derived from retail assembly; clean input set. */
/* Verified 9/9 words with the pinned oracle, gcc-2.7.2-psx / ASPSX 2.56, -O2 -G0. */
#include "psx_types.h"

extern u8 D_80063906[];
extern s32 D_80078EF4;

u16 func_8002A9DC(void) {
    return *(u16 *)(D_80063906 + (D_80078EF4 * 0x10));
}

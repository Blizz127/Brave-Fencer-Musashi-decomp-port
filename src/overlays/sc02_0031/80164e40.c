/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef union {
    u8 b[4];
    u32 w;
} Quad;

extern Quad D_801B6510;

s32 func_80164E40(void) {
    if (D_801B6510.b[0] != 0) {
        D_801B6510.b[0] = D_801B6510.b[0] - 0x10;
    }
    if (D_801B6510.b[1] != 0) {
        D_801B6510.b[1] = D_801B6510.b[1] - 0x10;
    }
    if (D_801B6510.b[2] != 0) {
        D_801B6510.b[2] = D_801B6510.b[2] - 0x10;
    }
    return D_801B6510.w == 0;
}

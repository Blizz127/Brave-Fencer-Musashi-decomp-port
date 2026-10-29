/* Owner re-derivation from retail assembly (clean input set). */
/* Stores 0x808080 through the fifth argument. Returns 0 for kind 0 or 5; for any other kind returns the table word at the signed index when that index is below the limit byte D_80115158[b].n, else 0. */
#include "psx_types.h"

typedef struct {
    u8 n;
    u8 pad1;
} Lim;

extern Lim D_80115158[];

s32 func_8017CEB8(u32 *tbl, s16 a, s16 b, s16 idx, u32 *out)
{
    s32 r;
    s32 lim;

    *out = 0x808080;
    lim = D_80115158[b].n;
    r = 0;
    switch (a) {
    case 0:
    case 5:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    default:
        if (idx < lim) {
            r = tbl[idx];
        }
        break;
    }
    return r;
}

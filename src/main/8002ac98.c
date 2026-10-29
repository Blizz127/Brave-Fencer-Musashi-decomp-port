/* Owner re-derivation from retail assembly (clean input set). */
/* Steps 64 table levels by a per-slot mode and a random delta, then clamps each to [limit, 4*limit]. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 pad[3];
} Lim8;

extern u8 D_80078EBA;
extern s16 D_800A6588[];
extern Lim8 D_8010F46C[];
extern s32 func_800291B4(s32);
extern s32 func_8005C49C(void);

void func_8002AC98(void)
{
    s16 *tbl = D_800A6588;
    s16 i;
    s16 val;
    s32 t;
    s16 r;
    s16 lim;

    if (D_80078EBA != 0) {
        return;
    }
    for (i = 0; i < 64; i++) {
        t = func_800291B4(i + 0x63) & 0xF;
        if (t == 0) {
            continue;
        }
        r = func_8005C49C() & 0x1F;
        switch (t) {
        case 1:
            switch (r) {
            default:
                val = tbl[i] + (r - 11);
                break;
            case 0:
            case 11:
                val = tbl[i] >> 1;
                break;
            }
            break;
        case 2:
            if (r != 0) {
                val = tbl[i] + (r - 11);
            }
            break;
        case 3:
            switch (r) {
            default:
                val = tbl[i] + (r - 11);
                break;
            case 0:
            case 11:
                val = tbl[i] << 1;
                break;
            }
            break;
        }
        lim = D_8010F46C[i + 1].x;
        if (val < lim) {
            tbl[i] = lim;
        } else if (lim * 4 < val) {
            tbl[i] = (s16)(lim << 2);
        } else {
            tbl[i] = val;
        }
    }
}

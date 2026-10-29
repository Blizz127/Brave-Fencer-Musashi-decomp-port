/* Owner re-derivation from retail assembly (clean input set). */
/* Fills the 16-byte record at D_800AF630 + 0x9DA8 + idx * 16 via func_80015424. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_80015424(void *rec, s32 w, s16 a, s16 b, u8 c, u8 d);

void func_800153CC(s16 idx, s32 w, s16 a, s16 b, u8 c, u8 d)
{
    register u8 *base = D_800AF630;

    func_80015424(base + (idx * 16 + 0x9DA8), w, a, b, c, d);
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Records the packed (table, entry) selector and returns the entry's 0x5A halfword. */
/* One function [80041400,80041448): splat's func_80041404 is the rest of this body. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x5A];
    s16 f5A;
    u8 pad5C[0xB0 - 0x5C];
} Entry;

extern u32 D_800C73D8[]; /* guest Entry * */
extern s16 D_800B9B36[];

s16 func_80041400(s32 sel)
{
    Entry *tbl = (Entry *)D_800C73D8[sel & 0xFF];

    D_800B9B36[0] = sel;
    tbl += (sel & 0xFF00) >> 8;
    return tbl->f5A;
}

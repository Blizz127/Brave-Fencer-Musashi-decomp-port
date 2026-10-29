/* Owner re-derivation from retail assembly (clean input set). */
/* Records the packed (table, entry) selector and returns the entry's 0x58 halfword. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x58];
    s16 f58;
    u8 pad5A[0xB0 - 0x5A];
} Entry;

extern u32 D_800C73D8[]; /* guest Entry * */
extern s16 D_800B9B36[];

s16 func_800413B8(s32 sel)
{
    Entry *tbl = (Entry *)D_800C73D8[sel & 0xFF];

    D_800B9B36[0] = sel;
    tbl += (sel & 0xFF00) >> 8;
    return tbl->f58;
}

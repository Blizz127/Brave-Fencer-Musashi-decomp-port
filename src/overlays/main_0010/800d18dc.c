/* Owner re-derivation from retail assembly (clean input set). */
/* Runs the state handler selected by the record's state byte D_800AE6BC out of a local copy of the three-entry handler table (guest code addresses) D_801141BC; returns the handler's result. */
#include "psx_types.h"

typedef struct {
    u32 w0;
    u32 w1;
    u32 w2;
} Rec800AE6A8;

typedef struct {
    u32 f[3];
} Tbl801141BC;

extern Rec800AE6A8 D_800AE6A8;
extern u8 D_800AE6BC;
extern Tbl801141BC D_801141BC;

s32 func_800D18DC(void)
{
    Tbl801141BC t;

    t = D_801141BC;
    return ((s32 (*)(Rec800AE6A8 *))t.f[D_800AE6BC])(&D_800AE6A8);
}

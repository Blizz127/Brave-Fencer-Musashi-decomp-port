/* Owner re-derivation from retail assembly (clean input set). */
/* Adds three deltas into entry idx (0..2) of the 16-byte-per-entry state table, then calls func_80028620 on it. */
#include "psx_types.h"

typedef struct { s32 w[4]; } WordRow;

extern WordRow D_800A5E88[];
extern WordRow D_800A5E8C[];
extern WordRow D_800A5E90[];
extern void func_80028620(u32 idx, WordRow *row);

void func_80028394(u16 idx, s32 a, s32 b, s32 c) {
    if (idx < 3) {
        D_800A5E88[idx].w[0] += a;
        D_800A5E8C[idx].w[0] += b;
        D_800A5E90[idx].w[0] += c;
        func_80028620(idx, &D_800A5E88[idx]);
    }
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Resets entry idx (0..2) of a 16-byte-per-entry state table to its initial values (0x64 in one word, zeros elsewhere) and calls func_80028620 on it. */
#include "psx_types.h"

typedef struct { s32 w[4]; } WordRow;
typedef struct { u8 b[16]; } ByteRow;

extern WordRow D_800A5E88[];
extern WordRow D_800A5E8C[];
extern WordRow D_800A5E90[];
extern ByteRow D_800A5E94[];
extern ByteRow D_800A5E95[];
extern ByteRow D_800A5E96[];
extern void func_80028620(u32 idx, WordRow *row);

void func_80028488(u16 idx) {
    if (idx < 3) {
        D_800A5E90[idx].w[0] = 0x64;
        D_800A5E88[idx].w[0] = 0;
        D_800A5E8C[idx].w[0] = 0;
        D_800A5E94[idx].b[0] = 0;
        D_800A5E95[idx].b[0] = 0;
        D_800A5E96[idx].b[0] = 0;
        func_80028620(idx, &D_800A5E88[idx]);
    }
}

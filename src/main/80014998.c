/* Owner re-derivation from retail assembly (clean input set). */
/* Copies three bytes from a fixed offset into each of two 0x5C-byte records. */
#include "psx_types.h"

typedef struct Rec5C {
    u8 pad0[0x51];
    u8 b51;
    u8 b52;
    u8 b53;
    u8 pad54[0x5C - 0x54];
} Rec5C;

typedef struct Block {
    u8 pad0[0x192];
    u8 b192;
    u8 b193;
    u8 b194;
} Block;

extern Block D_800AF630;

void func_80014998(void) {
    Block *blk;
    Rec5C *arr;
    s32 i;

    blk = &D_800AF630;
    arr = (Rec5C *)blk;
    for (i = 0; i < 2; i++) {
        arr[i].b51 = blk->b192;
        arr[i].b52 = blk->b193;
        arr[i].b53 = blk->b194;
    }
}

/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct { s16 a[3]; s16 p0; s16 b[2]; s16 c; s16 p1; u8 k[8]; u32 w; } Pk80168F40;
typedef struct { u32 w[8]; } Blk80168F40;
extern u8 D_800AE620[];
extern void func_80016A5C(void *a, void *b);

void func_80168F40(u8 *o)
{
    Pk80168F40 pk;
    Blk80168F40 blk;
    s32 pad[2];

    pk.a[0] = *(u16 *)(o + 0x10);
    pk.a[1] = *(u16 *)(o + 0x14);
    pk.a[2] = *(u16 *)(o + 0x18);
    pk.b[0] = *(u16 *)(o + 6);
    pk.b[1] = *(u16 *)(o + 0xA);
    pk.c = *(u16 *)(o + 0xE);
    pk.k[0] = 8;
    pk.k[2] = 0;
    pk.k[1] = 0;
    pk.k[4] = 0xC0;
    pk.k[6] = 0x80;
    pk.k[5] = 0x80;
    blk = (*(Blk80168F40 *)&D_800AE620);
    pk.w = 0x50000000;
    func_80016A5C(&pk, &blk);
}

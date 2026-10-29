/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up the 12-bit field of entry id (high halfword bits kept, bits 12-15 cleared) in the word table D_800D34AC and returns func_800191D4 of it. */
#include "psx_types.h"

extern s32 func_800191D4(s32 a);
extern u32 D_800D34AC[];

s32 func_800CFD68(s32 id)
{
    return func_800191D4(D_800D34AC[id & 0xFFFF0FFF] & 0xFFF);
}

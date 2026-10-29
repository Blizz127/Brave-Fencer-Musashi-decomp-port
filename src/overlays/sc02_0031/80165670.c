/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up D_80187970[a1 & 0x7F], latches it into D_80078EC8, and returns it (the caller reads
 * v0 right after the call, feeding the result into the next call's a1). a0 is accepted and
 * ignored. */
#include "psx_types.h"

extern s32 D_80187970[];
extern s32 D_80078EC8;

s32 func_80165670(s32 a0, u32 a1)
{
    s32 v = D_80187970[a1 & 0x7F];
    D_80078EC8 = v;
    return v;
}

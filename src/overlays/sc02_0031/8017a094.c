/* Owner re-derivation from retail assembly (clean input set). */
/* No jal callers found; v0 is not read after the call (clobbered on return), keeps the s32
 * ABI default. Stores a0 to D_801B751C, then runs func_80179B74 on a local u16 id (0x15). */
#include "psx_types.h"

extern s32 D_801B751C;
extern void func_80179B74(u16 *p);

s32 func_8017A094(s32 a0)
{
    u16 id = 0x15;
    D_801B751C = a0;
    func_80179B74(&id);
#ifndef __mips__
    return 0;
#endif
}

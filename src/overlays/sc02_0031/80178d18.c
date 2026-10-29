/* Owner re-derivation from retail assembly (clean input set). */
/* Clears D_801B7B0C then runs func_80174714(). Accepts (and ignores) a caller-supplied object
 * pointer. The lone sampled caller's post-call instruction overwrites v0 with an unrelated
 * immediate, so the call result is discarded; void abi. */
#include "psx_types.h"

extern s16 D_801B7B0C;
extern void func_80174714(void);

void func_80178D18(void *o)
{
    D_801B7B0C = 0;
    func_80174714();
}

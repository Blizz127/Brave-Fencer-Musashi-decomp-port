/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_8016DBD8(o), then dispatches o through the D_80187F6C function-pointer table,
 * indexed by the u16 field at o+2. */
#include "psx_types.h"

extern s32 func_8016DBD8(char *o);
extern void (*D_80187F6C[])(u8 *);

void func_8016DA30(u8 *o)
{
    func_8016DBD8((char *)o);
    D_80187F6C[*(u16 *)(o + 2)](o);
}

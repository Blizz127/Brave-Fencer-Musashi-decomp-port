/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatches &D_80126B58 through the D_80185FC0 function-pointer table, indexed by D_80126BA4. */
#include "psx_types.h"

extern u8 D_80126BA4;
extern u8 D_80126B58[];
extern void (*D_80185FC0[])(u8 *);

void func_80146360(void)
{
    D_80185FC0[D_80126BA4](D_80126B58);
}

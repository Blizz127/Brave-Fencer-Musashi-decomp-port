/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: starts func_80011A3C when func_800D0488(1) reports nonzero. */
#include "psx_types.h"

extern s32 func_800D0488(s32 a);
extern void func_80011A3C(void);

void func_800CEF04(void)
{
    if (func_800D0488(1) != 0) {
        func_80011A3C();
    }
}

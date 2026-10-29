/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: runs func_8001B324 then func_80011A3C. */
#include "psx_types.h"

extern void func_8001B324(void);
extern void func_80011A3C(void);

void func_800CEF34(void)
{
    func_8001B324();
    func_80011A3C();
}

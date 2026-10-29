/* Owner re-derivation from retail assembly (clean input set). */
/* func_8017E358(o), then dispatches o through D_80188944[o->2 (u16)], then if o->0x20 is set,
 * calls func_80162D28(o). */
#include "psx_types.h"

extern void func_8017E358(u8 *o);
extern void (*D_80188944[])(u8 *);
extern void func_80162D28(u8 *o);

void func_8017E0C0(u8 *o)
{
    func_8017E358(o);
    D_80188944[*(u16 *)(o + 2)](o);
    if (*(u32 *)(o + 0x20) != 0) {
        func_80162D28(o);
    }
}

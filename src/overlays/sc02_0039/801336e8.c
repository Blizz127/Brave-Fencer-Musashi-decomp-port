/* Install the mine room's floor grid and two additional descriptor references. */
#include "psx_types.h"

#if !defined(__mips__) || defined(__LP64__)
#include <stdint.h>
#endif

extern u32 D_8019F9B8;
extern u32 D_8019F9BC;
extern u32 D_8019F9C0;
extern void func_80136BC4(void *primary);

void func_801336E8(void *primary, u32 second, u32 third)
{
    if (primary) {
#if defined(__mips__) && !defined(__LP64__)
        D_8019F9B8 = (u32)primary;
#else
        D_8019F9B8 = (u32)(uintptr_t)primary;
#endif
        func_80136BC4(primary);
    }
    D_8019F9BC = second;
    D_8019F9C0 = third;
}

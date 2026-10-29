/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern void func_8015DAC4(char *o);
extern void func_8015554C(char *o);

void func_80149AD4(char *o)
{
    int pad[6];

    if (*(u32 *)(o + 0x178) != 0) {
        func_8015DAC4(o);
    } else {
        *(u8 *)(o + 0x1AA) = 1;
        func_8015554C(o);
    }
}

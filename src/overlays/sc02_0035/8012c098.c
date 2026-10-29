/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern void func_8012C218(char *o);

void func_8012C098(char *o)
{
    char *p = *(char **)(o + 0x68);
    if (p && (*(s16 *)(o + 0x72) & 0x8000)) {
        *(u16 *)(p + 0xA) &= 0x7FFF;
    }
    func_8012C218(o);
}

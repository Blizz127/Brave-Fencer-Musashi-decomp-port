/* Owner re-derivation from retail assembly (clean input set). */
/* func_800291C8(0x12, 0), then runs func_80016714(p, 0x38) for the first 3 entries of the
 * D_801B64B0 pointer table, walking the table with a pointer (matches retail's s0+=4 advance). */
#include "psx_types.h"

extern void func_800291C8(s32 a, s16 b);
extern void *D_801B64B0[];
extern void func_80016714(void *p, s32 n);

void func_80162760(void)
{
    void **p;
    s32 i;

    func_800291C8(0x12, 0);
    i = 0;
    p = D_801B64B0;
    while (i < 3) {
        func_80016714(*p, 0x38);
        p++;
        i++;
    }
}

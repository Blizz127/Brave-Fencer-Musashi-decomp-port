/* Owner re-derivation from retail assembly (clean input set). */
/* Clears a seed table entry (func_800291C8(8, 0)) then zeroes 0x38 bytes at each of the
 * 3 pointers in D_801B6458 via func_80016714 (the matched memset/bzero body). No caller
 * found in any region. */
#include "psx_types.h"

extern void func_800291C8(s32 i, s16 v);
extern void func_80016714(s8 *arg0, s32 arg1);
extern s8 *D_801B6458[3];

void func_8016216C(void)
{
    s32 i;

    func_800291C8(8, 0);
    i = 0;
    do {
        func_80016714(D_801B6458[i], 0x38);
        i++;
    } while (i < 3);
}

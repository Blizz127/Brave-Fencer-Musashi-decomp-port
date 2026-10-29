/* Owner re-derivation from retail assembly (clean input set). */
/* Fans each record's own stored value out through its own destination pointer,
 * walking a 0xC-byte record array that starts right after the object header
 * (records are {s32 *dst; s32 value; s32 unused;}).
 * abi s32: no jal callers or table references found for this symbol anywhere under
 * asm/ (any region or asm/main.s); default s32 per the ABI rule rather than void. */
#include "psx_types.h"

s32 func_8013C360(void *o)
{
    void *p;
    u32 i;

    p = (void *)((u8 *)o + 0x10);
    i = 0;
    while (i < *(u32 *)((u8 *)o + 8)) {
        *(*(s32 **)p) = *(s32 *)((u8 *)p + 4);
        p = (void *)((u8 *)p + 0xC);
        i++;
    }
#ifndef __mips__
    return 0;
#endif
}

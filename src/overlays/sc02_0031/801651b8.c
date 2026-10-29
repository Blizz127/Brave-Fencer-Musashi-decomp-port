/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_80165240(o, o+0x2C, o+0x50) once, then walks the 8-entry D_801B6550 table
 * (stride 0x10): for each entry whose +0xC timer is nonzero, runs
 * func_80165240(o, entry, entry+8) and decrements the timer. void, no caller found in
 * any region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xC];
    s32 timer;  /* 0xC */
    u8 pad10[0x10 - 0x10];
} Entry;

extern Entry D_801B6550[8];
extern void func_80165240(void *a, void *b, void *c);

void func_801651B8(u8 *o)
{
    s32 i;
    u8 *e;

    func_80165240(o, o + 0x2C, o + 0x50);

    i = 0;
    e = (u8 *)D_801B6550;
top:
    if (*(s32 *)(e + 0xC) != 0) {
        func_80165240(o, e, e + 8);
        *(s32 *)(e + 0xC) = *(s32 *)(e + 0xC) - 1;
    }
    i++;
    e += 0x10;
    if (i < 8) {
        goto top;
    }
}

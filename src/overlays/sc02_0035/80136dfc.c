/* Owner re-derivation from retail assembly (clean input set). */
/* Registers five records (indices 0xA-0xE) via func_800153CC, each with a different
 * halfword source forwarded as "w", a=0x110, and an increasing column constant "b".
 * abi s32: no jal callers or table references found for this symbol anywhere under
 * asm/ (any region or asm/main.s); default s32 per the ABI rule rather than void. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x12];
    s16 f12;
} Thing;

extern s16 D_80126B5E;
extern s16 D_80126B62;
extern s16 D_80126B66;
extern Thing *D_80126B78;
extern u16 D_80126CC4;
extern void func_800153CC(); /* no prototype in scope: args keep their own-type promotion,
                                 matching retail's lh/lh/lh/lh/lhu mix across the 5 calls */

s32 func_80136DFC(void)
{
    func_800153CC(0xA, D_80126B5E, 0x110, 0x3C, 4, 0);
    func_800153CC(0xB, D_80126B62, 0x110, 0x44, 4, 0);
    func_800153CC(0xC, D_80126B66, 0x110, 0x4C, 4, 0);
    func_800153CC(0xD, D_80126B78->f12, 0x110, 0x54, 4, 0);
    func_800153CC(0xE, D_80126CC4, 0x110, 0x5C, 4, 0);
#ifndef __mips__
    return 0;
#endif
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Copies the 10-byte record at D_801BC644 onto the stack and forwards it to func_8001534C.
 * abi s32 (table-dispatch rule): reached only via the jalr table entry at main_0012 8017E92C,
 * no direct jal callers found in any region or asm/main.s; default s32 per the dispatch-table
 * ABI rule rather than asserting void from the byte match alone. */
#include "psx_types.h"

typedef struct {
    s32 w0;
    s32 w1;
    s8 b0;
    s8 b1;
} __attribute__((packed)) Rec; /* 10 bytes */

extern u8 D_801BC644[];
extern void func_8001534C(s16 arg0, s32 arg1, s16 arg2, s16 arg3, u8 arg4, u8 arg5);

s32 func_80136C90(void)
{
    Rec local = *(Rec *)D_801BC644;
    func_8001534C(0, (s32)&local, 0x78, 0x10, 0, 0);
#ifndef __mips__
    return 0;
#endif
}

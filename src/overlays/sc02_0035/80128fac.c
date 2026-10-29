/* Owner re-derivation from retail assembly (clean input set). */
/* Two record-fill calls: func_8001534C(4, &D_801BC3F4, 0x80, 0x90, 0, 0) then
 * func_800153CC(5, o->f0, 0xA8, 0x90, 0x40, 0). No callers found; keeps the s32 ABI default. */
#include "psx_types.h"

extern u8 D_801BC3F4[];
extern void func_8001534C(s16 arg0, s32 arg1, s16 arg2, s16 arg3, u8 arg4, u8 arg5);
extern void func_800153CC(s16 idx, s32 w, s16 a, s16 b, u8 c, u8 d);

typedef struct {
    u16 f0;
} Obj;

s32 func_80128FAC(Obj *o)
{
    func_8001534C(4, (s32)D_801BC3F4, 0x80, 0x90, 0, 0);
    func_800153CC(5, o->f0, 0xA8, 0x90, 0x40, 0);
#ifndef __mips__
    return 0;
#endif
}

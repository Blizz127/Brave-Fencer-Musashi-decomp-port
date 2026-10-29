/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x15];
    u8 f15;       /* 0x15 */
    u8 pad16[0x19 - 0x16];
    u8 f19;       /* 0x19 */
} Obj;

extern void func_8016EDEC(void *a0, s32 a1);
extern u8 D_800D3348[];

s32 func_800D2E20(Obj *o) {
    func_8016EDEC(D_800D3348, 0x1000000);
    o->f19 = 0xA;
    o->f15 = o->f15 + 1;
    return 0;
}

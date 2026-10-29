/* Owner re-derivation from retail assembly (clean input set). */
/* If func_80171D1C(o, D_80126B30, D_80126B36) is nonzero, runs func_80171A1C(o); func_80174650(o)
 * [callee ignores o]. void abi: sampled caller's next instr is a plain epilogue restore. */
#include "psx_types.h"

extern u8 D_80126B36;
extern u8 D_80126B30[];
extern s32 func_80171D1C(void *o, s32 a, u8 c);
extern void func_80171A1C(void *o);
extern void func_80174650();

void func_80172958(void *o)
{
    if (func_80171D1C(o, (s32)D_80126B30, D_80126B36) != 0) {
        func_80171A1C(o);
        func_80174650(o);
    }
}

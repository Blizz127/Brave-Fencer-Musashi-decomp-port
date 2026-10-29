/* Owner re-derivation from retail assembly (clean input set). */
/* Thunk: func_801723C4(o, D_80127508); func_80171A1C(o); func_80174650() [callee ignores the
 * forwarded o]. void abi: the sampled caller's next instruction is a plain epilogue restore,
 * not a v0 read. */
#include "psx_types.h"

extern s32 D_80127508;
extern void func_801723C4(void *o, s32 a);
extern void func_80171A1C(void *o);
extern void func_80174650();

void func_80173770(void *o)
{
    func_801723C4(o, D_80127508);
    func_80171A1C(o);
    func_80174650(o);
}

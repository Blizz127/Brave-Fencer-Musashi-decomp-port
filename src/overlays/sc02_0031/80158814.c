/* Owner re-derivation from retail assembly (clean input set). */
/* Resets D_801B6440=0, D_801B644C=1, D_801B6450=0, o->fA8=0, func_80147084(o); func_80147098(o);
 * func_801470AC(o); func_80147078(o,0x1A); func_80158880(o) [already matched]. void abi: sampled
 * caller's next instr is a plain epilogue restore. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xA8];
    u8 fA8; /* 0xA8 */
} Obj;

extern s32 D_801B6440;
extern s32 D_801B644C;
extern s32 D_801B6450;
extern void func_80147084(void *o);
extern void func_80147098(void *o);
extern void func_801470AC(void *o);
extern void func_80147078(void *o, u16 a);
extern void func_80158880(void *o);

void func_80158814(Obj *o)
{
    D_801B6440 = 0;
    D_801B644C = 1;
    D_801B6450 = 0;
    o->fA8 = 0;
    func_80147084(o);
    func_80147098(o);
    func_801470AC(o);
    func_80147078(o, 0x1A);
    func_80158880(o);
}

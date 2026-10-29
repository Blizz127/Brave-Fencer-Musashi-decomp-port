/* Owner re-derivation from retail assembly (clean input set). */
/* func_80147324(0x18); o->f198 = func_8013767C(D_80187668[o->f1A0]); func_80146CA0(o). No caller
 * found in any available region/overlay; body never sets v0. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x198];
    u32 f198; /* 0x198 */
    u8 pad19C[0x1A0 - 0x19C];
    u8 f1A0;  /* 0x1A0 */
} Obj;

extern void func_80147324(u16 a);
extern u32 D_80187668[];
extern u32 func_8013767C(u32 a);
extern void func_80146CA0(void *o);

void func_8015FD74(Obj *o)
{
    func_80147324(0x18);
    o->f198 = func_8013767C(D_80187668[o->f1A0]);
    func_80146CA0(o);
}

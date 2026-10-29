/* Owner re-derivation from retail assembly (clean input set). */
/* func_80149020(o) then dispatches to the D_8018700C[o->f4F] handler table entry. void abi:
 * sampled caller's next instr is a plain epilogue restore, not a v0 read. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x4F];
    u8 f4F; /* 0x4F */
} Obj;

typedef void (*Handler)(Obj *);

extern void func_80149020(void *o);
extern u32 D_8018700C[];

void func_80158880(Obj *o)
{
    func_80149020(o);
    ((Handler)D_8018700C[o->f4F])(o);
}

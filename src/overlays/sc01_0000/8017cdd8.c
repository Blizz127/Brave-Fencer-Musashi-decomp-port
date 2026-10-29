/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_80019028 on the object, func_8017CFD4 and func_800167B8(0), advances the u8 phase at offset 0x15 and returns 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x15];
    u8 phase;
} Obj;

extern void func_80019028(Obj *o);
extern void func_8017CFD4(void);
extern void func_800167B8(s32 a);

s32 func_8017CDD8(Obj *o)
{
    func_80019028(o);
    func_8017CFD4();
    func_800167B8(0);
    o->phase++;
    return 0;
}

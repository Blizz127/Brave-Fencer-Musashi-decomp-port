/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8017CFFC, sets the s32 timer at offset 0x28 to 10, advances the u8 phase at offset 0x15 and returns 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x15];
    u8 phase;
    u8 pad16[0x12];
    s32 timer;
} Obj;

extern void func_8017CFFC(void);

s32 func_8017CE84(Obj *o)
{
    func_8017CFFC();
    o->timer = 10;
    o->phase++;
    return 0;
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x10];
    s32 f10, f14, f18;
    u8 pad1C[0x28];
    s32 f44, f48, f4C;
} Obj;

void func_8012B200(Obj *o) {
    o->f18 = 0;
    o->f14 = 0;
    o->f10 = 0;
    o->f4C = 0;
    o->f48 = 0;
    o->f44 = 0;
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x10];
    u32 f10;
    u32 f14;
    u32 f18;
    u8 pad2[0x44 - 0x1C];
    u32 f44;
    u32 f48;
    u32 f4C;
} Obj;

void func_8012B21C(Obj *o) {
    o->f18 = 0;
    o->f14 = 0;
    o->f10 = 0;
    o->f4C = 0;
    o->f44 = 0;
    o->f48 = 0xC000;
}

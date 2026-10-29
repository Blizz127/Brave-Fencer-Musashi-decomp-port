/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x14];
    s32 f14;
    u8 pad18[0x30];
    s32 f48;
    u8 pad4C[0xB6];
    s16 f102;
} Obj;

void func_80142C84(Obj *o) {
    o->f14 = 0xFFF40000;
    o->f48 = 0x20000;
    o->f102 = 0;
}

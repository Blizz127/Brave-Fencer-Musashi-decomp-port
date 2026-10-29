/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    u16 ang;
} Model;

typedef struct {
    u8 pad0[0x20];
    Model *m;
} Obj;

s32 func_80148CF4(Obj *o, s32 a) {
    o->m->ang = (a - 0x600) & 0xFFF;
    return 1;
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
typedef struct {
    u8 pad0[0x10];
    u16 f10;
} Inner;

typedef struct {
    u8 pad0[0x20];
    u32 p20;
    u8 pad24[0x5E - 0x24];
    u16 f5E;
    u8 pad60[0xC1 - 0x60];
    u8 fC1;
} Obj;

extern s32 func_80131CA8(Obj *o, s32 k);

/* Returns retail's leftover v0 (func_80131CA8's result). */
s32 func_80131C78(Obj *o) {
    o->fC1 = 0;
    ((Inner *)o->p20)->f10 = 0;
    o->f5E = 0;
    return func_80131CA8(o, 0xE);
}

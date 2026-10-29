/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[20];
    u16 t[1];
} Tab;

typedef struct {
    u8 pad0[0x78];
    Tab *tab;
} Obj;

u16 func_8014CA70(Obj *o, s32 i) {
    return o->tab->t[i];
}

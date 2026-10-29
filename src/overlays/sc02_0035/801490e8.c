/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x10A];
    u16 f10A;     /* 0x10A */
} Obj;

void func_801490E8(Obj *o, u16 a1) {
    o->f10A = a1;
}

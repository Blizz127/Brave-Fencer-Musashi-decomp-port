/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xFE];
    u16 fFE;      /* 0xFE */
} Obj;

void func_80182994(Obj *o) {
    o->fFE = 1;
}

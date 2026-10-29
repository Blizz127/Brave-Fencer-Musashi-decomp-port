/* Owner re-derivation from retail assembly (clean input set). */
/* Zeroes halfwords +0xAA, +0xAC and sets +0xAE to 0x8080. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xAA];
    u16 a;
    u16 b;
    u16 c;
} Obj;

void func_80148634(Obj *o) {
    o->a = 0;
    o->b = 0;
    o->c = 0x8080;
}

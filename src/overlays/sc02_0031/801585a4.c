/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x244];
    u16 f244;     /* 0x244 */
} Obj;

void func_801585A4(Obj *o) {
    o->f244 = 0;
}

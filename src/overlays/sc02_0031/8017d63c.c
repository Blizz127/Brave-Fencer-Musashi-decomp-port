/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2;       /* 0x2 */
    u8 pad4[0x2E - 0x4];
    u16 f2E;      /* 0x2E */
} Obj;

extern void func_80147324(u16 a0);

void func_8017D63C(Obj *o) {
    func_80147324(o->f2E);
    o->f2 = o->f2 + 1;
}

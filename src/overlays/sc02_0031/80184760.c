/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2;       /* 0x2 */
    u8 pad4[0xCC - 0x4];
    void *fCC;    /* 0xCC */
} Obj;

extern void func_8012C218(void *a0);

void func_80184760(Obj *o) {
    o->f2 = 2;
    func_8012C218(o->fCC);
}

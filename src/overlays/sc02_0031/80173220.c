/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xB8];
    u16 fB8;      /* 0xB8 */
} Obj;

extern void func_80147324(s32 a0);
extern void func_80019064(void *a0);
extern u8 D_801881E8[];
extern void func_80171A1C(Obj *o);
extern void func_80174650(Obj *o);

void func_80173220(Obj *o) {
    if ((o->fB8 & 0x4000) != 0) {
        func_80147324(0x516);
        func_80019064(D_801881E8);
    }
    if ((o->fB8 & 0x8000) != 0) {
        func_80171A1C(o);
        func_80174650(o);
    }
}

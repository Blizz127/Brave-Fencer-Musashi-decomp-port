/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xDA];
    u8 fDA;       /* 0xDA */
} Obj;

extern u8 D_8011520C[];
extern u16 D_80186F44[];
extern void func_80147324(u16 a0);

void func_80156A1C(Obj *o, u32 a1) {
    if (o->fDA == 0) {
        func_80147324(D_80186F44[D_8011520C[a1]]);
        o->fDA = o->fDA + 1;
    }
}

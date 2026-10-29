/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xB4];
    s32 fB4;
    u8 pad_c1[0xC1 - 0xB8];
    u8 fC1;
} Obj;

extern u8 D_80184840[];
extern void func_80131170(Obj *o, void *tbl, s32 n);
extern s32 func_80131CA8(Obj *o, s32 flag);

void func_8012F828(Obj *o) {
    o->fC1 = 4;
    if (o->fB4 & 8) {
        func_80131170(o, D_80184840, 0xB);
    }
    func_80131CA8(o, 9);
}

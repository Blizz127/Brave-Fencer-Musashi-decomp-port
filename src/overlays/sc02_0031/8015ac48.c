/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x16E];
    u16 f16E;     /* 0x16E */
} Obj;

extern void func_80154150(Obj *o, s32 a1);
extern void func_80146DB8(Obj *o, void *a1);
extern void func_80161CD0(void *o, u16 a1);
extern u8 D_801872A0[];

void func_8015AC48(Obj *o) {
    func_80154150(o, 9);
    func_80146DB8(o, D_801872A0);
    func_80161CD0(o, o->f16E);
}

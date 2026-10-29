/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x170];
    s32 f170;     /* 0x170 */
} Obj;

extern void func_801553C0(Obj *o);
extern void func_80154A74(Obj *o, s32 a1);

void func_8015CB94(Obj *o) {
    o->f170 = 0;
    func_801553C0(o);
    func_80154A74(o, 0x11);
}

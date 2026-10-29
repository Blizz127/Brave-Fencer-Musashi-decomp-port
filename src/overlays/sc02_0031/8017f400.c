/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    s32 f44;      /* 0x44 */
    u8 pad48[0xB8 - 0x48];
    u16 fB8;      /* 0xB8 */
} Obj;

extern void func_801466F0(s32 a0, Obj *a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7);
extern void func_80147324(s32 a0);
extern void func_80171990(Obj *o);

void func_8017F400(Obj *o) {
    if ((o->fB8 & 0x4000) != 0) {
        o->f44 |= 0x20000;
        func_801466F0(0x16, o, 0, -0x80, -0x10, 1, 0, 0);
        func_80147324(0x7BC);
        func_80171990(o);
    }
}

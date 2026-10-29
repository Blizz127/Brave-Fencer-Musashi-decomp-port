/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 f0;       /* 0x0 */
} Obj;

extern void func_8001534C(s32 a0, void *a1, s32 a2, s32 a3, s32 a4, s32 a5);
extern void func_800153CC(s32 a0, u16 a1, s32 a2, s32 a3, s32 a4, s32 a5);
extern u8 D_801B5D58[];

void func_80165E2C(Obj *o) {
    func_8001534C(0, D_801B5D58, 0x80, 0x80, 0, 0);
    func_800153CC(1, o->f0, 0xA8, 0x80, 0x40, 0);
}

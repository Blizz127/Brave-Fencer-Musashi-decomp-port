/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x27];
    u8 f27;       /* 0x27 */
} Sub;

typedef struct {
    u8 pad0[0xCC];
    Sub *fCC;     /* 0xCC */
} Obj;

extern u16 D_800B99DA;
extern u8 D_80189354[];
extern u8 D_801B4A44[];
extern void func_800183E0(void *a0);

void func_80183A20(Obj *o) {
    o->fCC->f27 = 0xA3;
    func_800183E0(D_801B4A44 + (D_80189354[D_800B99DA & 0x1F] << 4));
}

/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x20];
    void *f20;    /* 0x20 */
    u8 f24[4];    /* 0x24 */
} Obj;

extern s32 func_80128ED8(void *a0, void *a1);
extern void func_801292C8(Obj *o);

void func_80182BE0(Obj *o) {
    if (func_80128ED8(o->f20, &o->f24) == 1) {
        func_801292C8(o);
    }
}

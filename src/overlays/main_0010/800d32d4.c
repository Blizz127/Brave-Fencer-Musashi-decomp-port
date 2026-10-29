/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x198];
    void *f198;   /* 0x198 */
    u8 pad19C[0x214 - 0x19C];
    u8 f214;      /* 0x214 */
} Obj;

extern s32 func_801399F0(void *a0);
extern void func_80139914(void *a0);

void func_800D32D4(Obj *o) {
    if (func_801399F0(o->f198) != 0) {
        func_80139914(o->f198);
        o->f214 = o->f214 + 1;
    }
}

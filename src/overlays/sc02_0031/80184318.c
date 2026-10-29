/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2;       /* 0x2 */
} Obj;

extern s32 func_80178970(Obj *o);
extern void func_80178D18(Obj *o);

void func_80184318(Obj *o) {
    if (func_80178970(o) != 0) {
        func_80178D18(o);
        o->f2 = 1;
    }
}

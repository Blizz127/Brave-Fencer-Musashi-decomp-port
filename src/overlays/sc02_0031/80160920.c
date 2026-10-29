/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u16 f0;       /* 0x0 */
    u8 pad2[0xB8 - 0x2];
    u16 fB8;      /* 0xB8 */
};

extern s32 func_801487F4(Obj *o);
extern Fn D_8018730C[];
extern void func_80160A74(Obj *o);
extern s32 func_80161208(Obj *o);
extern void func_80154150(Obj *o, s32 a1);
extern void func_80146CA0(Obj *o);

void func_80160920(Obj *o) {
    if ((func_801487F4(o) & 8) == 0) {
        D_8018730C[o->f0](o);
        func_80160A74(o);
    } else if (func_80161208(o) == 0 && o->fB8 == 0x8000) {
        func_80154150(o, 0x25);
        func_80146CA0(o);
    }
}

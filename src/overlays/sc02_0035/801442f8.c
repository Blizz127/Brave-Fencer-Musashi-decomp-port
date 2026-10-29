/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u8 pad0[2];
    u16 f2;       /* 0x2 */
    u8 pad4[0x1C - 0x4];
    s32 f1C;      /* 0x1C */
};

extern Fn D_80185E6C[];
extern void func_8012C218(Obj *o);

void func_801442F8(Obj *o) {
    D_80185E6C[o->f2](o);
    o->f1C = o->f1C + 1;
    if (o->f1C >= 0x1E) {
        func_8012C218(o);
    }
}

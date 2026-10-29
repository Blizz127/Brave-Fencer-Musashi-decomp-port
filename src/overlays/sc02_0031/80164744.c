/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u8 pad0[2];
    u16 f2;       /* 0x2 */
    u8 pad4[0x20 - 0x4];
    void *f20;    /* 0x20 */
};

extern Fn D_80187874[];
extern void func_80162D28(Obj *o);

void func_80164744(Obj *o) {
    Fn fn = D_80187874[o->f2];
    void *x = o->f20;

    fn(o);
    if (x != 0) {
        func_80162D28(o);
    }
}

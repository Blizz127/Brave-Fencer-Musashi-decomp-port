/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u8 pad0[0xDC];
    Fn fDC;       /* 0xDC */
};

void func_80144A04(Obj *o) {
    o->fDC(o);
}

/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u8 pad0[0x215];
    u8 f215;      /* 0x215 */
};

extern Fn D_80188198[];

void func_80170C74(Obj *o) {
    D_80188198[o->f215](o);
}

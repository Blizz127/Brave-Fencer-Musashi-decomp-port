/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x118];
    s16 v[3][4];
} Obj801474EC;

void func_801474EC(Obj801474EC *o) {
    o->v[0][2] = 0;
    o->v[0][1] = 0;
    o->v[0][0] = 0;
    o->v[1][2] = 0;
    o->v[1][1] = 0;
    o->v[1][0] = 0;
    o->v[2][2] = 0;
    o->v[2][1] = 0;
    o->v[2][0] = 0;
}

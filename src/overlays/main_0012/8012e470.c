/* Owner re-derivation from retail assembly (clean input set). */
/* Eases the +0x18 value of the sub-record toward 0x1000 by a quarter; counts down +0x1C and returns 1 when it reaches zero. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x18];
    s16 cur;
    s16 pad1A;
    s16 prev;
} Sub;

typedef struct {
    u8 pad0[0x1C];
    s32 count;
    Sub *sub;
} Obj;

s32 func_8012E470(Obj *o) {
    Sub *s = o->sub;
    s32 t = s->cur;
    s32 c = t;
    s32 v;
    s32 n;
    s32 r;
    v = c + (0x1000 - t) / 4;
    s->prev = v;
    s->cur = v;
    n = o->count;
    if (n == 0) {
        r = 1;
    } else {
        n--;
        o->count = n;
        r = n == 0;
    }
    return r;
}

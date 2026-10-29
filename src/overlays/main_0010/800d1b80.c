/* Owner re-derivation from retail assembly (clean input set). */
/* Runs the record's step callback; when it reports done, advances the record's state byte and runs func_800D1BD8. Returns 0 while the callback is not done. */
#include "psx_types.h"

typedef struct Rec800D1B80 {
    u8 pad00[0xC];
    u32 step;
    u8 pad10[4];
    u8 state;
} Rec800D1B80;

extern s32 func_800D1BD8(Rec800D1B80 *r);

s32 func_800D1B80(Rec800D1B80 *r)
{
    s32 res;

    if (((s32 (*)(struct Rec800D1B80 *))r->step)(r) == 0) {
        res = 0;
    } else {
        r->state = r->state + 1;
        res = func_800D1BD8(r);
    }
    return res;
}

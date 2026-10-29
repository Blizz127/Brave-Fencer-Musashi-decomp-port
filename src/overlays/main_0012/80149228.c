/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame repeat/hold tracker: latches the new input mask (and newly-changed bits) and ticks the s16 timer down. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xAC];
    u16 input; /* 0xAC */
    u8 pad1[0x1DC - 0xAE];
    u32 edge; /* 0x1DC */
    u32 prev; /* 0x1E0 */
    s16 timer; /* 0x1E4 */
    u16 reload; /* 0x1E6 */
} Obj80149228;

void func_80149228(Obj80149228 *o)
{
    u32 in;
    u32 t;

    if (o->timer == 0) {
        o->edge = 0;
        o->prev = 0;
        o->timer = o->reload;
    }
    in = o->input;
    if (in != 0) {
        t = in & o->prev;
        o->prev = in;
        o->timer = o->reload;
        o->edge = t;
    }
    o->timer = o->timer - 1;
}

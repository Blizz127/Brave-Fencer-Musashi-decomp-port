/* Owner re-derivation from retail assembly (clean input set). */
/* Steps an interpolation counter: yields 1 when func_80139220 reports done, otherwise writes the next output point and advances the counter. */
#include "psx_types.h"
typedef struct { u8 p0[0x12]; u16 f12; u16 f14; u8 p16[0x16]; s16 f2C; u8 p2E[0xA]; u16 f38; u16 f3A; } O;
extern s32 func_80139220(O *o);
s32 func_8013914C(O *o, u16 *out) {
    if (o->f14 >= o->f2C) {
        if (func_80139220(o)) return 1;
    }
    out[0] = o->f38 + ((u32)(o->f14 * 3) >> 1);
    out[1] = o->f3A + (o->f12 * 3 << 2);
    o->f14++;
    return 0;
}

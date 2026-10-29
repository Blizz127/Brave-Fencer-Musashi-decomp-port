/* func_80148824 - owner re-derivation from retail assembly (clean input set).
 * Length of the (hi-0x80, lo-0x80) byte offset pair at +0xAE: |d| when one
 * axis is zero, otherwise func_80047D3C(x*x + y*y). Returned as a byte. */
#include "psx_types.h"

extern s32 func_80047D3C(s32 v);

u8 func_80148824(u8 *o) {
    u16 h;
    s32 x;
    s32 y;
    s16 ax; /* s16 copies: retail keeps x/y copies in a1/a0 for the abs paths */
    s16 ay;
    h = *(u16 *)(o + 0xAE);
    x = (h >> 8) - 0x80;
    ax = x;
    y = (h & 0xFF) - 0x80;
    ay = y;
    if (x == 0) {
        if (y < 0) {
            return -ay;
        }
        return ay;
    }
    if (y == 0) {
        if (x < 0) {
            return -ax;
        }
        return ax;
    }
    return func_80047D3C(x * x + y * y);
}

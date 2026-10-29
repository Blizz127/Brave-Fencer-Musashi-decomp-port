/* func_8015640C - owner re-derivation, clean room (nmO).
 * Each arm returns its whole expression: the division sits in the arm (the
 * shared magic-load `lui` and the cross-jumped mult tail come from jump2), and
 * the arm's result feeding the return register suggests v0 for the quotient in
 * local-alloc. `& 0xFFFF` instead of a (u16) cast keeps andi before ori (a cast
 * is shortened to a u16 ior). The zero arm is written first for the retail
 * `beqz` layout. callers read v0 (s32). */
#include "psx_types.h"
extern u16 func_801564B0(void *o);
extern u8 func_80029178(s32 a);

s32 func_8015640C(void *o, u16 b) {
    if (func_801564B0(o) == 0) return b;
    if (func_80029178(0x1C) == 0) {
        return ((b * 150 / 100) & 0xFFFF) | 0x8000;
    }
    return ((b * 175 / 100) & 0xFFFF) | 0x8000;
}

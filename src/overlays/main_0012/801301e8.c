/* func_801301E8 - owner re-derivation, clean room (nmO).
 * The two call pairs are written per arm, both passing o explicitly: CSE drops
 * the redundant `a0 = o` in the C3 arm (a0 still holds o from entry) and jump2
 * cross-jumps the tails, giving retail's `j` past the `move a0,s0`.
 * func_80130D48 reads a0 (o) - passed explicitly, no implicit-argument hazard.
 * callers read v0: none (void). */
#include "psx_types.h"

extern s32 func_8012BEE8(u8 *o);
extern void func_80130D48(u8 *o);
extern void func_801319E0(u8 *o);
extern void func_80131CA8(u8 *o, s32 a);

void func_801301E8(u8 *o) {
    if (*(u32 *)(o + 0xB4) & 0x2000) {
        if (o[0xC3] != 0) {
            if (*(u16 *)(o + 0x72) & 0x4000) {
                func_80130D48(o);
                func_801319E0(o);
            }
        } else if (func_8012BEE8(o) != 0) {
            func_80130D48(o);
            func_801319E0(o);
        }
    }
    func_80131CA8(o, 0x26);
}

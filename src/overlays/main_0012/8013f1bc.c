/* func_8013F1BC - owner re-derivation from retail assembly (clean input set).
 * Counts the D_80115118 timer down by 0x10; once it is zero, either calls
 * func_800D2624 (D_80115114 == D_800B9A02 ^ 1) or bumps D_80115112.
 *
 * Two sequential ifs: the join label between the store and the second test
 * stops CSE from forwarding the stored value, so retail re-reads the timer.
 * D_80115118 is s16 in other owner sources; the u16 view is a cast here
 * (its computed address also gives retail's lui/addiu base register). */
#include "psx_types.h"

extern u16 D_80115118;
extern u16 D_80115114;
extern u16 D_800B9A02;
extern u16 D_80115112;
extern void func_800D2624(void);

void func_8013F1BC(void) {
    if (*(u16 *)((s16 *)&D_80115118) != 0) {
        *(u16 *)((s16 *)&D_80115118) -= 0x10;
    }
    if (*(u16 *)((s16 *)&D_80115118) != 0) {
        return;
    }
    if (D_80115114 == (D_800B9A02 ^ 1)) {
        func_800D2624();
    } else {
        D_80115112++;
    }
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Clears the overlay-ready flag in the global state block and calls the overlay entry func_800CEDFC. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_800CEDFC(void);

void func_8001125C(void) {
    register u8 *state = D_800AF630;

    state[0xA434] = 0;
    func_800CEDFC();
}

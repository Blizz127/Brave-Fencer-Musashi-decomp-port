/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80011778();                            /* extern */

extern u8 D_800AF630[];
void func_80010DA0(void) {
    register u8 *unused = D_800AF630;
    func_80011778();
}

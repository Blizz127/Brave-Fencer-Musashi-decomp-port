/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

s32 func_800291B4(s32);                               /* extern */
void func_8002D4C8(s32, s32);                              /* extern */

void func_8001B710(void) {
    if ((func_800291B4(0x44) & 0xFF) >= 2) {
        func_8002D4C8(0x24, 0);
    }
    if ((func_800291B4(0x56) & 0xFF) >= 2) {
        func_8002D4C8(0x25, 0);
    }
    if ((func_800291B4(0x5C) & 0xFF) >= 2) {
        func_8002D4C8(0x26, 0);
    }
}

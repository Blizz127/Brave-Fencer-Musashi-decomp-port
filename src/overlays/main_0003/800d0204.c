/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_800118AC();                            /* extern */
void func_800D0D6C();                            /* extern */
void func_800D1E58();                            /* extern */
extern s32 D_800EC8B0;

void func_800D0204(void) {
    if (D_800EC8B0 != 0) {
        func_800D1E58();
        func_800D0D6C();
        func_800118AC();
    } else {
        func_800118AC();
    }
}

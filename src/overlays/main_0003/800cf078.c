/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_800118AC();                            /* extern */
void func_800CF3E8();                            /* extern */
s32 func_800CFC1C(s32 *);                       /* extern */
extern u8 D_800DA48C[];
extern s32 D_800EC690;
extern s32 D_800EC6A0;
extern s32 D_800EC6A4;

void func_800CF078(void) {
    func_800CF3E8();
    if (D_800EC6A0 == 0) {
        if (func_800CFC1C(D_800DA48C) == 0) {
            D_800EC6A0 = 1;
        }
        if (D_800EC6A0 != 0) {
            goto block_4;
        }
    } else {
block_4:
        if (D_800EC6A4 != 0) {
            D_800EC690 = 1;
            func_800118AC();
        }
    }
}

/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u8 D_8011F738;
extern s32 func_800D0F8C(u8 a0);
extern s32 D_8011F73C;
extern s32 D_8011F740;
extern void func_801718AC(s32 a0);

void func_80170EAC(void) {
    s32 a0;
    if (func_800D0F8C(D_8011F738) != 0) {
        a0 = D_8011F73C;
    } else {
        a0 = D_8011F740;
    }
    func_801718AC(a0);
}

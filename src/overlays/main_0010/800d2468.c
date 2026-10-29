/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_80016714(void *p, s32 n);
extern u8 D_80115110[];
extern s32 D_8012751C;
extern s32 D_8011DB20;

void func_800D2468(void) {
    func_80016714(D_80115110, 0xC4);
    D_8012751C = 0;
    D_8011DB20 = 0;
}

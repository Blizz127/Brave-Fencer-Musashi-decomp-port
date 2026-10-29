/* Owner re-derivation from retail assembly (clean input set). */
/* Reverse of sibling func_800D1F58: restores the halfwords D_80078EB4/D_80078EB8 and the
 * word D_80078E8C from the saved words D_8011DB14/D_8011DB1C/D_80128150. */
#include "psx_types.h"

extern s32 D_80078E8C;
extern u16 D_80078EB4;
extern u16 D_80078EB8;
extern s32 D_8011DB14;
extern s32 D_8011DB1C;
extern s32 D_80128150;

void func_800D1F90(void)
{
    D_80078EB4 = D_8011DB14;
    D_80078EB8 = D_8011DB1C;
    D_80078E8C = D_80128150;
}

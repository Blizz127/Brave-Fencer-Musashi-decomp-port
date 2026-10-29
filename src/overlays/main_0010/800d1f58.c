/* Owner re-derivation from retail assembly (clean input set). */
/* Copies three words of state: the halfwords D_80078EB4 and D_80078EB8 and the word D_80078E8C. */
#include "psx_types.h"

extern s32 D_80078E8C;
extern u16 D_80078EB4;
extern u16 D_80078EB8;
extern s32 D_8011DB14;
extern s32 D_8011DB1C;
extern s32 D_80128150;

void func_800D1F58(void)
{
    D_8011DB14 = D_80078EB4;
    D_8011DB1C = D_80078EB8;
    D_80128150 = D_80078E8C;
}

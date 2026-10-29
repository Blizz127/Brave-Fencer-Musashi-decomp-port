/* Owner re-derivation from retail assembly (clean input set). */
/* Calls the handler whose address is held in D_8011DB24. */
#include "psx_types.h"

extern u32 D_8011DB24;

void func_800D25FC(void)
{
    ((void (*)(void))D_8011DB24)();
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct { u8 b[0x10]; } Ent8013A860;
extern u8 D_8011DA80[];
extern void func_80016714(void *, s32);

void func_8013A860(void) {
    s32 i;
    for (i = 0; i < 2; i++) {
        func_80016714(&((Ent8013A860 *)&D_8011DA80)[i], 0x10);
    }
}

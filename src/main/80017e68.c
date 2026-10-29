/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

void func_80017E68(void *arg0, void *arg1) {
    (*(s32 *) ((s8 *) (arg1) + (0x14))) = (s32) (*(s16 *) ((s8 *) (arg0) + (0)));
    (*(s32 *) ((s8 *) (arg1) + (0x18))) = (s32) (*(s16 *) ((s8 *) (arg0) + (2)));
    (*(s32 *) ((s8 *) (arg1) + (0x1C))) = (s32) (*(s16 *) ((s8 *) (arg0) + (4)));
}

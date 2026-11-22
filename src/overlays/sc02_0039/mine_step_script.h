#ifndef BFM_MINE_STEP_SCRIPT_H
#define BFM_MINE_STEP_SCRIPT_H

#include "psx_types.h"

extern u32 D_8019FA30;
extern u8 D_8019FA34, D_8019FA35;
extern s32 D_801269D4, D_801269C8;

#ifndef __mips__
/* Load one byte of guest backing at the point when the game reads it. */
u8 bfm_mine_script_read(u32 address);
#endif

void func_8013CA14(void);
#endif

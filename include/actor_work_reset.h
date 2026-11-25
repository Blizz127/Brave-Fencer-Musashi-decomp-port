#ifndef BFM_ACTOR_WORK_RESET_H
#define BFM_ACTOR_WORK_RESET_H

#include "psx_types.h"

/* The matching PS1 routine leaves 200 in V0. Host callers can read it. */
#ifdef __mips__
void func_8001C9D0(u8 *record);
#else
u32 func_8001C9D0(u8 *record);
#endif

#endif

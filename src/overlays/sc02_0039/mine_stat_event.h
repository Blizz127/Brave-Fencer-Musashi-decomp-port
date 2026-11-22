#ifndef BFM_MINE_STAT_EVENT_H
#define BFM_MINE_STAT_EVENT_H

#include "psx_types.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define MINE_STAT_ACTOR_READY(actor) (*(u16 *)((actor) + 0xb8) & 0x8000u)
#else
#define MINE_STAT_ACTOR_READY(actor) ((actor)[0xb9] & 0x80u)
#endif

void func_80174158(void *actor);
void func_80174188(void *actor);
void func_801741A8(void *actor);

#endif

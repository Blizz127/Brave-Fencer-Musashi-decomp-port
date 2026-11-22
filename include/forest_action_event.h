#ifndef MUSASHI_FOREST_ACTION_EVENT_H
#define MUSASHI_FOREST_ACTION_EVENT_H

#include "forest_action.h"
#include "forest_event.h"

#if defined(__mips__) && !defined(__LP64__)
#define FOREST_EVENT_ACTOR(address) ((ForestPlayerState *)(address))
#else
/* Resolve a live actor handle or stop the enclosing transaction. */
ForestPlayerState *bfm_forest_event_actor(u32 address);
#define FOREST_EVENT_ACTOR(address) bfm_forest_event_actor(address)
#endif

s32 func_8014BEF8(ForestPlayerState *actor);
s32 func_80029178(u32 bit);
#endif

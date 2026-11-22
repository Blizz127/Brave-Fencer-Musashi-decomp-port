#ifndef BFM_STEAM_KNIGHT_ACTOR84_H
#define BFM_STEAM_KNIGHT_ACTOR84_H

#include "steam_knight_hit.h"

void func_80189CCC(SteamKnightPart *actor);
void func_8018BA38(SteamKnightPart *actor);
void func_8018BAE4(SteamKnightPart *actor);

#if !defined(__mips__) || defined(__LP64__)
/* Model and resource values crossing these room services are guest addresses. */
u32 bfm_tower_actor84_allocate_model(void);
void bfm_tower_actor84_dispose(SteamKnightPart *actor);
void bfm_tower_actor84_model_list(u32 model_address, u32 list_address);
void bfm_tower_actor84_animation(SteamKnightPart *actor, u32 script_address);
#endif

#endif

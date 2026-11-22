#ifndef BFM_STEAM_KNIGHT_ARMOR_SETUP_H
#define BFM_STEAM_KNIGHT_ARMOR_SETUP_H

#include "steam_knight_hit.h"

void func_80184EC0(SteamKnightPart *part);

#if !(defined(__mips__) && !defined(__LP64__))
void bfm_tower_armor_dispose(u8 *actor);
void bfm_tower_armor_bind_model(void *model);
s32 bfm_tower_armor_advance(u8 *actor);
#endif

#endif

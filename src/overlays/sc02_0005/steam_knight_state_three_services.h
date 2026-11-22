#ifndef BFM_STEAM_KNIGHT_STATE_THREE_SERVICES_H
#define BFM_STEAM_KNIGHT_STATE_THREE_SERVICES_H

#include "steam_knight_hit.h"

typedef struct {
    u16 id;
    u8 p2[4];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0xfc];
} SteamKnightSpawnParent;

extern s32 func_8012BEE8(SteamKnightPart *body);
extern s32 func_8012C588(s16 kind, SteamKnightSpawnParent *parent);

#endif

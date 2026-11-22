#ifndef BFM_STEAM_KNIGHT_HIT_H
#define BFM_STEAM_KNIGHT_HIT_H

#include "psx_types.h"

/* Fields touched when a Steam Knight attachment consumes a pending hit. */
typedef struct {
    u8 pad000[28];
    s32 timer;
    u8 pad020[60];
    u16 flags;
    u16 hit_kind;
    s16 damage;
    u8 pad062[20];
    s16 hp;
    u8 pad078[73];
    u8 reaction;
    u8 pad0C2[74];
} SteamKnightPart;

void func_801853EC(SteamKnightPart *part);
void func_8016AA50(SteamKnightPart *part, int amount);
void func_80185060(SteamKnightPart *part, int mode);
void func_80186B08(SteamKnightPart *part);
void func_80186A04(SteamKnightPart *part);
void func_8002D4C8(u16 command, u16 argument);

#endif

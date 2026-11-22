#ifndef BFM_STEAM_KNIGHT_HIT_H
#define BFM_STEAM_KNIGHT_HIT_H

#include "psx_types.h"

typedef struct {
    u8 bytes[8];
} SteamKnightHitPosition;

/* Fields touched when a Steam Knight attachment consumes a pending hit. */
typedef struct {
    u16 active;
    u8 pad002[26];
    s32 timer;
    u8 pad020[60];
    u16 flags;
    u16 hit_kind;
    s16 damage;
    u16 contact_parameter;
    u8 pad064[18];
    s16 hp;
    u8 pad078[4];
    SteamKnightHitPosition hit_position;
    u8 pad084[61];
    u8 reaction;
    u8 pad0C2[6];
    u8 contact_tag;
    u8 contact_frame;
    u8 pad0CA[66];
} SteamKnightPart;

SteamKnightPart *func_8014C4AC(SteamKnightPart *part, int kind, int amount,
                               const void *position, u16 parameter);
void func_801853EC(SteamKnightPart *part);
void func_8016AA50(SteamKnightPart *part, int amount);
void func_80185060(SteamKnightPart *part, int mode);
void func_80186B08(SteamKnightPart *part);
void func_80186A04(SteamKnightPart *part);
void func_8002D4C8(u16 command, u16 argument);

#endif

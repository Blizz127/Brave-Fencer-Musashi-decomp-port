#ifndef BFM_STEAM_KNIGHT_EFFECT_H
#define BFM_STEAM_KNIGHT_EFFECT_H
#include "psx_types.h"

typedef struct {
    u16 kind;
    s16 x, y, z;
    u32 owner_address;
    s32 mode, amount;
} SteamKnightEffectRequest;

typedef struct {
    u16 kind;
    u8 pad002[4];
    s16 x;
    u8 pad008[2];
    s16 y;
    u8 pad00C[2];
    s16 z;
    u8 pad010[28];
    s32 mode, amount;
    u32 owner_address;
    u8 pad038[32];
} SteamKnightEffect;

SteamKnightEffect *func_80146A6C(int kind, u32 owner_address, int x, int y,
                                 s16 z, int mode, int amount);
SteamKnightEffect *func_80146AFC(const SteamKnightEffectRequest *request);

#endif

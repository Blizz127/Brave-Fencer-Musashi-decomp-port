#ifndef BFM_STEAM_KNIGHT_MODEL_BINDING_H
#define BFM_STEAM_KNIGHT_MODEL_BINDING_H

#include "psx_types.h"

/* Keep this parameter compatible with the game-owned resident model initializer. */
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x20];
    s32 unk24;
} SteamKnightModelBinding;

void func_8001C2C4(SteamKnightModelBinding *model);

#endif

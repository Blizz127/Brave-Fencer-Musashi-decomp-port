/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_H
#define BFM_STEAM_KNIGHT_H
#include "steam_knight_field.h"
#include "forest_voice.h"
#include "gameplay_run.h"
typedef struct BfmSteamKnight BfmSteamKnight;
/* The drawbridge submits its destination once. Preserve the player's run. */
BfmSteamKnight *bfm_steam_knight_open(const MusashiDiscMedia *disc,
    const RoomTransferDescriptor *request, BfmGameplayRun *run);
void bfm_steam_knight_free(BfmSteamKnight *encounter);
int bfm_steam_knight_tick(BfmSteamKnight *encounter, uint16_t pressed, uint16_t held);
int bfm_steam_knight_control(const BfmSteamKnight *encounter);
uint16_t bfm_steam_knight_room(const BfmSteamKnight *encounter);
unsigned bfm_steam_knight_shade(const BfmSteamKnight *encounter);
int bfm_steam_knight_hero(const BfmSteamKnight *encounter, int16_t feet[3],
                          int16_t *heading, int *running, unsigned *motion_tick);
uint32_t bfm_steam_knight_body(const BfmSteamKnight *encounter);
int bfm_steam_knight_draw(BfmSteamKnight *encounter, const BfmForestPose *pose,
    const BfmForestTransform *root, uint32_t visible_parts, const int8_t fusion[2][3],
    const int8_t lumina[2][3], uint32_t pixels[240][320]);
ForestVoice *bfm_steam_knight_voice(BfmSteamKnight *encounter, unsigned *generation);
#endif

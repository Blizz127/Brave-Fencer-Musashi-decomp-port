/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_BOSS_HUD_H
#define BFM_STEAM_KNIGHT_BOSS_HUD_H
#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t name[24];
    unsigned count;
} BfmSteamKnightBossHud;

/* Resolve the room's own name through the mounted disc's field alphabet. */
int bfm_steam_knight_boss_hud_load(BfmSteamKnightBossHud *hud, const MusashiDiscMedia *disc,
                                  const uint8_t *scene, size_t size);

/* The room owns the mounted field tiles. Draw after the field and before
 * the next dialogue takes focus. The encounter owns the title interval. */
void bfm_steam_knight_boss_title(const uint16_t tiles[512][1024],
                                 uint32_t pixels[240][320]);
/* Pass the live chassis HP and the number of 30 Hz field updates since
 * battle control was released. The four armor targets have separate HP. */
void bfm_steam_knight_boss_status(const BfmSteamKnightBossHud *hud, const uint16_t tiles[512][1024],
                                  unsigned body_hp, unsigned age, uint32_t pixels[240][320]);
#endif

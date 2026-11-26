/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_BOSS_HUD_H
#define BFM_STEAM_KNIGHT_BOSS_HUD_H
#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t name[24];
    unsigned count;
    unsigned drawn_width, queued_width, current_width;
} BfmSteamKnightBossHud;

/* Resolve the room's own name through the mounted disc's field alphabet. */
int bfm_steam_knight_boss_hud_load(BfmSteamKnightBossHud *hud, const MusashiDiscMedia *disc,
                                  const uint8_t *scene, size_t size);

/* The room owns the mounted field tiles. Draw after the field and before
 * the next dialogue takes focus. The encounter owns the title interval. */
void bfm_steam_knight_boss_title(const uint16_t tiles[512][1024],
                                 uint32_t pixels[240][320]);
/* Submit the live chassis HP once per completed 30 Hz battle update. */
void bfm_steam_knight_boss_hud_tick(BfmSteamKnightBossHud *hud, unsigned body_hp);

/* The age in battle updates controls the opening reveal. Rendering can be repeated. */
void bfm_steam_knight_boss_status(const BfmSteamKnightBossHud *hud, const uint16_t tiles[512][1024],
                                  unsigned age, uint32_t pixels[240][320]);
#endif

/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_HUD_H
#define BFM_FOREST_HUD_H

#include "forest_status.h"
#include "forest_terrain.h"

typedef struct BfmForestHud BfmForestHud;

/* The decoded SC01/77 payload can be released immediately. The mounted
 * terrain graphics must outlive the HUD. New forest entry starts hidden. */
BfmForestHud *bfm_forest_hud_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                 size_t bytes, const BfmForestTerrain *terrain);
BfmForestHud *bfm_bedroom_hud_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                  size_t bytes, const uint16_t tiles[512][1024]);
BfmForestHud *bfm_steam_knight_hud_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                        size_t bytes, const BfmForestTerrain *terrain);
BfmForestHud *bfm_village_hud_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                  size_t bytes, const uint16_t tiles[512][1024]);
void bfm_bedroom_hud_hide(BfmForestHud *hud);
/* COMMAND leaves the status panel on screen, moving its charge meter offscreen. */
void bfm_castle_hud_command(BfmForestHud *hud);
void bfm_castle_hud_command_closed(BfmForestHud *hud);
void bfm_forest_hud_free(BfmForestHud *hud);
/* The doorway actor requests display when it releases ordinary control.
 * Advance once on that 30 Hz update and once per subsequent update. */
void bfm_forest_hud_show(BfmForestHud *hud);
void bfm_forest_hud_tick(BfmForestHud *hud);
/* Supply current speed charge and a 0..128 absorption meter, or -1
 * when the player has no tether. Both actions use the same HUD well. */
void bfm_forest_hud_gauges(BfmForestHud *hud, unsigned charge, int absorption);
/* Sample HP and BP after the forest simulation update. The bar and its
 * digits share the value submitted two updates before the displayed frame. */
void bfm_forest_hud_submit_points(BfmForestHud *hud, const BfmForestStatus *status);
void bfm_forest_hud_draw(const BfmForestHud *hud, const BfmForestStatus *status,
                         uint32_t pixels[240][320]);

#endif

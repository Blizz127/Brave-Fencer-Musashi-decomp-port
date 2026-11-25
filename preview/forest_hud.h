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
void bfm_forest_hud_free(BfmForestHud *hud);
/* The doorway actor requests display when it releases ordinary control.
 * Advance once on that 30 Hz update and once per subsequent update. */
void bfm_forest_hud_show(BfmForestHud *hud);
void bfm_forest_hud_tick(BfmForestHud *hud);
void bfm_forest_hud_draw(const BfmForestHud *hud, const BfmForestStatus *status,
                         uint32_t pixels[240][320]);

#endif

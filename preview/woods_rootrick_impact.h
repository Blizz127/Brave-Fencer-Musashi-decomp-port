/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_IMPACT_H
#define BFM_WOODS_ROOTRICK_IMPACT_H

#include "bedroom_figure.h"
#include "hero_smoke_material.h"

typedef struct {
    BfmHeroSmokeMaterial smoke;
    const uint8_t *shake;
    int16_t center[3];
    unsigned puff_age, puffs_active, next_sample, hold, shaking;
    int camera_y, next_camera_y;
} BfmWoodsRootrickImpact;

/* Room borrows the mounted SC02/11 shake track. MAIN is the ordinary resident
 * player-data view supplied to the room; its smoke texture is in the field tiles. */
int bfm_woods_rootrick_impact_load(BfmWoodsRootrickImpact *out, const uint8_t *scene,
                                   size_t bytes, const uint8_t *main, size_t main_bytes);

/* During each of Rootrick's two landing events. The authored landings are
 * farther apart than either sprite's complete lifetime. */
void bfm_woods_rootrick_impact_hit(BfmWoodsRootrickImpact *impact, const int16_t xyz[3]);
/* Restart the same camera track at a rival-race or post-win landing. */
void bfm_woods_rootrick_impact_shake(BfmWoodsRootrickImpact *impact);
/* Run once per field, after the encounter actor and before draw. The render
 * camera uses the previous shake sample; this tick prepares the next one. */
void bfm_woods_rootrick_impact_tick(BfmWoodsRootrickImpact *impact);
/* Add this to both eye Y and target Y of the main field view, before building
 * its matrix. The tracked camera, HUD and screen-space writing keep their own coordinates. */
int bfm_woods_rootrick_impact_camera_y(const BfmWoodsRootrickImpact *impact);
/* Submit both at their field depth while the normal room ordering table is open. */
int bfm_woods_rootrick_impact_submit(const BfmWoodsRootrickImpact *impact,
                                     BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif

/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_FUSION_ART_H
#define BFM_WOODS_FUSION_ART_H

#include "bedroom_figure.h"
#include "bedroom_motion.h"
#include "forest_tether_trails.h"
#include "forest_damage_digits.h"
#include "woods_fusion.h"
#include "woods_foot_dust.h"
#include "forest_strike_effects.h"
#include "woods_charge_core.h"

/* Apply to a copy of the ordinary body pose. The cast/award clip supplies the
 * mounted joint address and grip. A zero address preserves the movement pose.
 * MAIN is the borrowed type-0x101 payload already used by bedroom_motion. */
int bfm_woods_fusion_art_pose(const uint8_t *main, size_t main_bytes, uint32_t pose,
                              uint32_t visible, int blade_away, BfmBedroomPose *body);

/* Convert the current field projection for world-space actor effects. */
int bfm_woods_effect_camera(const BfmBedroomView *view, BfmForestCamera *camera);

/* After preparing the player and caster, append the flight owner's current
 * sword and ribbons, including ribbons still returning after a catch. The
 * optional BP digits belong to an incomplete absorption's actual refund. */
int bfm_woods_fusion_art_append(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmForestActor *blade, const BfmForestTetherTrails *trails,
                                const BfmForestDamageDigits *bp_digits);

struct BfmWoodsChargedFx;
int bfm_woods_fusion_art_cut(BfmBedroomFigure *figure, const BfmBedroomView *view,
                            const struct BfmWoodsChargedFx *effects);

int bfm_woods_fusion_art_lights(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmWoodsFusionFrame *frame);

typedef struct {
    BfmWoodsChargeCore core;
    BfmForestTransform model;
    int8_t anchors[2][3];
    unsigned tick, gauge;
} BfmWoodsChargeArt;

int bfm_woods_charge_art_append(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmSteamChargeEffects *effects, const BfmWoodsFootDust *foot,
                                const BfmWoodsChargeArt *art);

/* Charged blade contacts in Cleft Woods use the shared field impact. The
 * mounted scene outlives these records. A wave crossing has no blade impact. */
typedef struct {
    BfmForestTransform face;
    int16_t point[3], velocity[3];
    unsigned age;
    int trail;
} BfmWoodsTerrainSpark;

typedef struct {
    BfmForestImpactArt art;
    const uint8_t *fork_orientations;
    BfmWoodsTerrainSpark terrain[BFM_FOREST_STRIKE_SLOTS];
    BfmForestStrikeEffects pool;
    BfmForestDustCard cards[BFM_FOREST_STRIKE_CARDS];
    unsigned count;
} BfmWoodsChargedImpact;

int bfm_woods_charged_impact_load(BfmWoodsChargedImpact *impact, const uint8_t *scene, size_t bytes);
int bfm_woods_charged_impact_issue(BfmWoodsChargedImpact *impact, const int16_t point[3],
                                  const int16_t normal[3], const int32_t eye[3]);
/* blocked_actor selects the original alternate displacement when a solid
 * actor alone blocked the blade. Issue after resolving body-hit precedence. */
int bfm_woods_charged_impact_terrain(BfmWoodsChargedImpact *impact, const int16_t point[3],
                                    const int16_t normal[3], int blocked_actor);
int bfm_woods_charged_impact_step(BfmWoodsChargedImpact *impact, BfmGameplayRun *run,
                                 const BfmBedroomView *view);
int bfm_woods_charged_impact_add(const BfmWoodsChargedImpact *impact, BfmBedroomFigure *figure,
                                const BfmBedroomView *view);

#endif

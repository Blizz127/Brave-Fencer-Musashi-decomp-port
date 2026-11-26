/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_FUSION_ART_H
#define BFM_WOODS_FUSION_ART_H

#include "bedroom_figure.h"
#include "bedroom_motion.h"
#include "forest_tether_trails.h"

/* Apply to a copy of the ordinary body pose. The cast/award clip supplies the
 * mounted joint address and grip. A zero address preserves the movement pose.
 * MAIN is the borrowed type-0x101 payload already used by bedroom_motion. */
int bfm_woods_fusion_art_pose(const uint8_t *main, size_t main_bytes, uint32_t pose,
                              uint32_t visible, int blade_away, BfmBedroomPose *body);

/* After preparing the player and caster, append the flight owner's current
 * sword and ribbons, including ribbons still returning after a catch. */
int bfm_woods_fusion_art_append(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmForestActor *blade, const BfmForestTetherTrails *trails);

#endif

/* SPDX-License-Identifier: MIT */
#include "forest_shot_impact.h"
#include "forest_impact.h"
#include <string.h>

static void move_emitter(BfmForestShotImpact *impact)
{
    for (unsigned i = 0; i < 3; ++i)
        impact->point[i] = (int16_t)(impact->point[i] + impact->offset[i]);
}

static BfmForestShotEmission emit(const BfmForestShotImpact *impact, unsigned kind, int variant)
{
    BfmForestShotEmission birth = {.kind = kind, .variant = variant};
    memcpy(birth.position, impact->point, sizeof birth.position);
    memcpy(birth.angles, impact->angles, sizeof birth.angles);
    return birth;
}

size_t bfm_forest_shot_impact_begin(BfmForestShotImpact *impact, const int16_t contact[3],
    const int16_t normal[3], BfmForestShotEmission out[BFM_FOREST_SHOT_EMISSIONS])
{
    if (!impact || !contact || !normal || !out) return 0;
    int pitch, yaw;
    bfm_forest_impact_angles(normal, &pitch, &yaw);
    BfmForestTransform basis;
    bfm_forest_actor_pitch_yaw((unsigned)pitch, (unsigned)yaw, contact, &basis);
    *impact = (BfmForestShotImpact){.angles = {(int16_t)pitch,(int16_t)yaw,0}, .remaining = 2};
    memcpy(impact->point, contact, sizeof impact->point);
    for (unsigned row = 0; row < 3; ++row) {
        int component = -4 * basis.rotation[row][1] - 8 * basis.rotation[row][2];
        impact->offset[row] = (int16_t)(component / 4096 - (component % 4096 < 0));
    }
    move_emitter(impact);
    out[0] = emit(impact, 9, 3);
    out[1] = out[0];
    move_emitter(impact);
    out[2] = emit(impact, 9, 2);
    out[3] = emit(impact, 3, -5);
    return 4;
}

size_t bfm_forest_shot_impact_tick(BfmForestShotImpact *impact,
    BfmForestShotEmission out[BFM_FOREST_SHOT_EMISSIONS])
{
    if (!impact || !impact->remaining || !out) return 0;
    move_emitter(impact);
    out[0] = emit(impact, 9, --impact->remaining ? 3 : 2);
    return 1;
}

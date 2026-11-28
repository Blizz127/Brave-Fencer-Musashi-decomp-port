/* SPDX-License-Identifier: MIT */
#include "village_mine_basket.h"

int16_t bfm_mine_basket_step_sway(BfmMineBasketSway *sway)
{
    int angle = sway->angle_q16 >> 16;
    int32_t velocity = sway->velocity_q16;
    if (angle < -2) velocity += 2 * 65536;
    if (angle > 2) velocity -= 2 * 65536;
    if (velocity > 0) velocity = velocity > 32768 ? velocity - 32768 : 0;
    else if (velocity < 0) velocity = velocity < -32768 ? velocity + 32768 : 0;
    sway->velocity_q16 = velocity;
    sway->angle_q16 += velocity;
    return (int16_t)(sway->angle_q16 >> 16);
}

void bfm_mine_basket_body_root(const BfmForestTransform *visual, BfmForestTransform *body)
{
    *body = *visual;
    for (int axis = 0; axis < 3; ++axis)
        body->translation[axis] += (visual->rotation[axis][1] * 144) >> 12;
}

void bfm_mine_basket_carry(BfmMineBasketSway *sway, const BfmForestGuardBox *box,
                           const BfmForestTransform *previous_body, const BfmForestTransform *body,
                           int ridden, int32_t foot_q16[3])
{
    if (ridden) {
        int32_t relative[3], local[3];
        for (int axis = 0; axis < 3; ++axis)
            relative[axis] = foot_q16[axis] - previous_body->translation[axis] * 65536;
        for (int axis = 0; axis < 3; ++axis) {
            int64_t dot = 0;
            for (int world = 0; world < 3; ++world)
                dot += (int64_t)previous_body->rotation[world][axis] * relative[world];
            local[axis] = (int32_t)(dot >> 12);
        }
        if (!sway->ridden) sway->velocity_q16 += local[0] >> 2;
        local[1] = (box->low[1] - 1) * 65536;
        for (int axis = 0; axis < 3; ++axis) {
            int64_t dot = 0;
            for (int coord = 0; coord < 3; ++coord)
                dot += (int64_t)body->rotation[axis][coord] * local[coord];
            foot_q16[axis] = body->translation[axis] * 65536 + (int32_t)(dot >> 12);
        }
    }
    sway->ridden = ridden != 0;
}

BfmMineBasketContact bfm_mine_basket_probe(const BfmForestGuardBox *box,
    const BfmForestTransform *body, const int16_t previous[3], const int16_t candidate[3],
    const int16_t last_supported[3], int16_t foot[3])
{
    /* The load-bearing child stores a local actor Y of 144 even as its
     * submitted box travels in world space. The actor query checks both. */
    if (candidate[1] > 144) return BFM_MINE_BASKET_MISS;
    int same_height = previous[1] == candidate[1];
    int stationary = same_height && previous[0] == candidate[0] && previous[2] == candidate[2];
    const int16_t *origin = last_supported ? last_supported : previous;
    int16_t start[3] = {origin[0], origin[1], origin[2]};
    int16_t end[3] = {candidate[0], candidate[1], candidate[2]};
    if (last_supported || stationary) start[1] -= 3;
    if (stationary) end[1] += 5;
    else if (last_supported) end[1] += 8;

    if (stationary && last_supported) {
        int16_t center[3];
        return bfm_forest_guard_contact(box, body, start, end, center) ?
               BFM_MINE_BASKET_KEEP : BFM_MINE_BASKET_MISS;
    }
    if (!bfm_forest_body_slide(box, body, start, end, foot)) return BFM_MINE_BASKET_MISS;
    if (last_supported) return BFM_MINE_BASKET_RIDE;
    return same_height && !stationary ? BFM_MINE_BASKET_TOUCH : BFM_MINE_BASKET_BOARD;
}

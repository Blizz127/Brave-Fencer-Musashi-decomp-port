/* SPDX-License-Identifier: MIT */
#include "woods_slope.h"
#include "forest_motion.h"

void bfm_woods_slope_begin(BfmWoodsSlope *s, const int16_t normal[3], unsigned countdown)
{
    s->downhill_q16[0] = normal[0] * 64;
    s->downhill_q16[1] = 8 * 65536;
    s->downhill_q16[2] = normal[2] * 64;
    s->remaining = countdown;
    s->settling = 0;
}

static int32_t component(int32_t a, int32_t x, int32_t b, int32_t z)
{
    int64_t v = (int64_t)a*x + (int64_t)b*z;
    return (int32_t)(v < 0 ? -((-v+4095)/4096) : v/4096);
}

void bfm_woods_slope_move(BfmWoodsSlope *s, int steering, unsigned yaw,
                          int32_t delta[3], int32_t local[3])
{
    /* Recover the ordinary level basis on unit vectors. Positive and negative
     * sine coefficients round independently when the columns are normalized. */
    ForestPlayerState basis = {0};
    int16_t angle = (int16_t)(yaw & 4095);
    basis.field_24.word = 4096;
    bfm_forest_level_velocity(&basis,0,angle,0);
    int32_t back = (int32_t)basis.velocity[2];
    basis.field_24.word = 0;
    basis.field_2c.word = 4096;
    bfm_forest_level_velocity(&basis,0,angle,0);
    int32_t side = (int32_t)basis.velocity[0];
    int32_t front = (int32_t)basis.velocity[2];
    if (steering) {
        s->downhill_q16[0] = (int32_t)((uint32_t)s->downhill_q16[0] - (uint32_t)(4*side));
        s->downhill_q16[2] = (int32_t)((uint32_t)s->downhill_q16[2] - (uint32_t)(4*front));
    }

    /* The action retains its downhill vector in world coordinates, sets
     * local velocity at the new facing, then uses ordinary player movement. */
    int32_t x = s->downhill_q16[0], z = s->downhill_q16[2];
    local[0] = component(front,x,back,z);
    local[1] = s->downhill_q16[1];
    local[2] = component(side,x,front,z);
    delta[0] = component(front,local[0],side,local[2]);
    delta[1] = local[1];
    delta[2] = component(back,local[0],front,local[2]);
}

BfmWoodsSlopeResult bfm_woods_slope_contact(BfmWoodsSlope *s, unsigned terrain,
                                           int actor_support, const int16_t normal[3])
{
    if (!terrain)
        return actor_support ? BFM_WOODS_SLOPE_CONTINUE : BFM_WOODS_SLOPE_FALL;

    int steep = s->settling ? (terrain & 0x4000) != 0 : (terrain & 0xff00) == 0x4000;
    if (s->settling && steep) {
        bfm_woods_slope_begin(s,normal,s->remaining);
    } else if (!s->settling && !steep) {
        s->settling = 1;
        s->remaining = 6;
    } else {
        if (s->remaining) --s->remaining;
        if (!s->remaining) {
            if (s->settling) return BFM_WOODS_SLOPE_IDLE;
            bfm_woods_slope_begin(s,normal,0);
        }
    }
    return BFM_WOODS_SLOPE_CONTINUE;
}

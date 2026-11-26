/* SPDX-License-Identifier: MIT */
#include "forest_flight.h"
#include "forest_shore_motion.h"
#include <math.h>

static int floor_shift(int64_t value, int divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

static void rotate(BfmForestTransform *model, unsigned axis, int angle)
{
    if (!(angle & 4095)) return;
    double radians = (angle & 4095) * (6.28318530717958647693 / 4096.0);
    int sine = (int)lround(sin(radians) * 4096);
    int cosine = (int)lround(cos(radians) * 4096);
    unsigned a = (axis + 1) % 3, b = (axis + 2) % 3;
    for (unsigned row = 0; row < 3; ++row) {
        int u = model->rotation[row][a], v = model->rotation[row][b];
        model->rotation[row][a] = (int16_t)floor_shift((int64_t)u * cosine + (int64_t)v * sine, 4096);
        model->rotation[row][b] = (int16_t)floor_shift((int64_t)v * cosine - (int64_t)u * sine, 4096);
    }
}

/* The blade's initial tilt is applied in X/Z/Y order. The live orientation
 * then steers its axis and spins its model about that axis. */
static void orient(BfmFusionFlight *f, int outbound)
{
    f->model = (BfmForestTransform){.rotation={{4096,0,0},{0,4096,0},{0,0,4096}}};
    if (outbound) {
        rotate(&f->model, 0, f->tilt[0]);
        rotate(&f->model, 2, f->tilt[2]);
        rotate(&f->model, 1, f->tilt[1]);
    }
    rotate(&f->model, 1, f->yaw);
    rotate(&f->model, 0, f->pitch);
    rotate(&f->model, 2, f->spin);
}

static void advance(BfmFusionFlight *f)
{
    for (unsigned i = 0; i < 3; ++i) {
        int32_t v = floor_shift((int64_t)f->model.rotation[i][2] * f->speed, 4096);
        /* Room positions are wrapping signed 16.16. */
        uint32_t next = (uint32_t)f->position[i] + (uint32_t)v;
        f->velocity[i] = v;
        f->position[i] = next <= INT32_MAX ? (int32_t)next : (int32_t)((int64_t)next - 4294967296LL);
        f->model.translation[i] = floor_shift(f->position[i], 65536);
    }
}

int bfm_fusion_launch(BfmFusionFlight *f, const BfmForestTransform *root,
                      const int16_t angles[3], const int16_t tilt[3])
{
    if (!f || !root || !angles || !tilt ||
        (f->phase != BFM_FUSION_IDLE && f->phase != BFM_FUSION_DONE)) return 0;
    BfmFusionFlight launch = {
        .phase = BFM_FUSION_OUTBOUND, .speed = -32 * 65536, .timer = 16,
        .pitch = angles[0], .yaw = angles[1], .spin = angles[2], .model = *root
    };
    for (unsigned i = 0; i < 3; ++i) {
        int64_t coordinate = root->translation[i] + (int64_t)floor_shift(-30 * root->rotation[i][1], 4096);
        if (coordinate < INT16_MIN || coordinate > INT16_MAX) return 0;
        launch.model.translation[i] = (int32_t)coordinate;
        launch.position[i] = (int32_t)coordinate * 65536;
        launch.tilt[i] = tilt[i];
    }
    *f = launch;
    return 1;
}

static int angle_delta(int target, int from)
{
    int d = (target & 4095) - (from & 4095);
    return d > 2048 ? d - 4096 : d < -2048 ? d + 4096 : d;
}

static int16_t turn(int from, int target, int divisor)
{
    int d = angle_delta(target, from);
    int step = d / divisor;
    if (!step && d) {
        /* Outbound steering chooses its minimum from the original angle
         * pair. The faster return turn follows the short arc. */
        int direction = divisor == 10 ? target - from : d;
        step = direction < 0 ? -1 : 1;
    }
    return (int16_t)((from + step) & 4095);
}

static void return_step(BfmFusionFlight *f, const int16_t owner[3])
{
    int x = owner[0] - f->model.translation[0];
    int y = f->model.translation[1] + 30 - owner[1];
    int z = owner[2] - f->model.translation[2];
    f->yaw = turn(f->yaw, (int)bfm_forest_shore_bearing(x, z), 2);
    f->pitch = turn(f->pitch, z ? (int)bfm_forest_shore_bearing(y, z < 0 ? -z : z) : 0, 2);
    f->spin = (int16_t)((f->spin + 113) & 4095);
    f->speed += 32768;
    orient(f, 0);
    advance(f);
}

void bfm_fusion_release_tether(BfmFusionFlight *f)
{
    if (f && f->phase == BFM_FUSION_ATTACHED) {
        f->phase = BFM_FUSION_RETURNING;
        f->timer = 32;
    }
}

BfmFusionFlightPhase bfm_fusion_flight_tick(BfmFusionFlight *f, const int16_t owner[3],
    const int16_t aim[3], BfmFusionContact contact, void *context)
{
    if (!f) return BFM_FUSION_IDLE;
    if (!owner) return f->phase;
    if (f->phase == BFM_FUSION_OUTBOUND) {
        int16_t from[3], to[3];
        for (unsigned i = 0; i < 3; ++i) from[i] = (int16_t)f->model.translation[i];
        if (aim) {
            int heading = (int)bfm_forest_shore_bearing(from[0] - aim[0], from[2] - aim[2]);
            int gap = (heading - f->yaw) & 4095;
            if (gap >= 2048) gap = 4095 - gap;
            if (gap < 512) f->yaw = turn(f->yaw, heading, 10);
        }
        f->spin = (int16_t)((f->spin + 113) & 4095);
        f->speed += 98304;
        orient(f, 1);
        advance(f);
        for (unsigned i = 0; i < 3; ++i) to[i] = (int16_t)f->model.translation[i];
        int hit = f->speed < 0 && contact ? contact(context, from, to) : 0;
        if (hit || f->speed >= 0) {
            f->phase = hit > 0 ? BFM_FUSION_ATTACHED : BFM_FUSION_TURNING;
            f->speed = 12 * 65536;
        }
    } else if (f->phase == BFM_FUSION_TURNING) {
        f->phase = BFM_FUSION_RETURNING;
        f->timer = 32;
    } else if (f->phase == BFM_FUSION_RETURNING || f->phase == BFM_FUSION_CATCHING) {
        return_step(f, owner);
        if (f->phase == BFM_FUSION_CATCHING) {
            if (!--f->timer) f->phase = BFM_FUSION_DONE;
        } else {
            int64_t distance2 = 0;
            for (unsigned i = 0; i < 3; ++i) {
                int delta = owner[i] - f->model.translation[i];
                distance2 += (int64_t)delta * delta;
            }
            if (distance2 < 40 * 40 || !--f->timer) {
                f->phase = BFM_FUSION_CATCHING;
                f->timer = 8;
            }
        }
    }
    return f->phase;
}

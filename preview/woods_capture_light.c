/* SPDX-License-Identifier: MIT */
#include "woods_capture_light.h"
#include "forest_bullet_art.h"
#include <math.h>

enum { RIM = 12, FACES = RIM / 2 };

static int down(int64_t n, int divisor)
{
    return (int)(n / divisor - (n % divisor < 0));
}

static void advance_motes(BfmWoodsCaptureLight *light)
{
    for (unsigned i = 0; i < BFM_WOODS_CAPTURE_MOTES; ++i) {
        BfmWoodsCaptureMote *mote = &light->motes[i];
        if (!mote->remaining || !--mote->remaining) continue;
        for (unsigned axis = 0; axis < 3; ++axis) {
            mote->position[axis] += mote->velocity[axis];
            mote->velocity[axis] -= down(mote->velocity[axis], 16);
        }
    }
}

static void scatter(BfmWoodsCaptureLight *light, BfmTetherRandom random, void *context)
{
    const int16_t origin[3] = {0};
    unsigned emitted = 0;
    for (unsigned i = 0; i < BFM_WOODS_CAPTURE_MOTES && emitted < 5; ++i) {
        BfmWoodsCaptureMote *mote = &light->motes[i];
        if (mote->remaining) continue;
        unsigned draw = random(context);
        int pitch = (int)(draw & 127) * 8 - 128;
        unsigned yaw = ((draw >> 8) & 127) * 32;
        BfmForestTransform direction;
        bfm_forest_actor_pitch_yaw((unsigned)(pitch < 0 ? -pitch : pitch), yaw, origin, &direction);
        mote->scale = 512 + (int)(draw & 0x3f0);
        mote->remaining = 16;
        for (unsigned axis = 0; axis < 3; ++axis) {
            int forward = direction.rotation[axis][2];
            if (axis == 1 && pitch < 0) forward = -forward;
            mote->position[axis] = (int64_t)light->center[axis] * 65536;
            mote->velocity[axis] = down(576 * forward, 4096) * 4096;
        }
        ++emitted;
    }
}

static void place_light(BfmWoodsCaptureLight *light, const int32_t eye[3])
{
    BfmForestTransform biased;
    bfm_forest_glow_facing(eye, light->anchor, 6, &biased);
    for (unsigned axis = 0; axis < 3; ++axis) light->center[axis] = biased.translation[axis];
    light->show = 1;
}

void bfm_woods_capture_light_tick(BfmWoodsCaptureLight *light, const int16_t target_root[3],
    int earned, const int32_t eye[3], BfmTetherRandom random, void *rng_context)
{
    if (!light || !eye) return;
    advance_motes(light);
    light->show = 0;
    if (light->held && earned) {
        light->held = 0;
        light->burst_ticks = 8;
        light->scale_x = 4096;
        light->scale_y = 5120;
    } else if (light->burst_ticks) {
        if (!--light->burst_ticks) return;
        int expanding = light->burst_ticks >= 4;
        int change = expanding ? 2048 : -3072;
        light->scale_x += change;
        light->scale_y += change;
        place_light(light, eye);
        if (expanding && random) scatter(light, random, rng_context);
    } else if (target_root && !earned) {
        if (light->held) {
            light->scale_x ^= 1024;
            light->scale_y ^= 512;
            place_light(light, eye);
        } else {
            light->held = 1;
            for (unsigned axis = 0; axis < 3; ++axis) light->anchor[axis] = target_root[axis];
            light->anchor[1] -= 40;
            light->scale_x = 4096;
            light->scale_y = 5120;
        }
    } else {
        light->held = 0;
    }
}

/* The radius is sampled in the first quadrant. Reflect those rounded
 * coordinates so diametrically opposite tips have exactly the same reach. */
static void circle(int radius, int16_t rim[RIM][2])
{
    const double turn = 6.28318530717958647693 / 4096.0;
    for (unsigned i = 0; i <= 3; ++i) {
        double angle = turn * (341 * i);
        rim[i][0] = (int16_t)down(radius * lround(4096 * sin(angle)), 4096);
        rim[i][1] = (int16_t)down(radius * lround(4096 * cos(angle)), 4096);
    }
    for (unsigned i = 4; i <= 6; ++i) {
        rim[i][0] = rim[6 - i][0];
        rim[i][1] = (int16_t)-rim[6 - i][1];
    }
    for (unsigned i = 7; i < RIM; ++i) {
        rim[i][0] = (int16_t)-rim[RIM - i][0];
        rim[i][1] = rim[RIM - i][1];
    }
}

/* These circles stay parallel to the screen. The field projects their world
 * center and their scaled local vertices independently at the center's Z. */
static size_t project_circle(const BfmForestCamera *camera, const int32_t center[3],
    const int16_t rim[RIM][2], int scale_x, int scale_y, uint32_t color,
    BfmForestDustCard *out, size_t capacity)
{
    BfmForestCamera screen = {
        .rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}, .projection = camera->projection
    };
    int16_t points[4][3] = {{0}};
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t sum = 0;
        for (unsigned k = 0; k < 3; ++k) sum += (int64_t)camera->rotation[axis][k] * center[k];
        int64_t world = (int64_t)camera->translation[axis] + down(sum, 4096);
        if (world < INT16_MIN || world > INT16_MAX) return 0;
        points[0][axis] = (int16_t)world;
    }
    for (unsigned i = 1; i < 4; ++i) points[i][2] = points[0][2];
    BfmForestDustCard projection = {0};
    if (!bfm_forest_project_effect_quad(&screen, points, &projection)) return 0;
    unsigned bucket = projection.bucket + 1;
    if (bucket >= 4096) return 0;
    BfmForestRasterVertex hub = projection.vertices[0];
    hub.rgb = color;
    BfmForestRasterVertex tips[RIM + 1];
    for (unsigned i = 0; i < RIM; i += 4) {
        for (unsigned k = 0; k < 4; ++k) {
            points[k][0] = (int16_t)down(rim[i + k][0] * scale_x, 4096);
            points[k][1] = (int16_t)down(rim[i + k][1] * scale_y, 4096);
        }
        if (!bfm_forest_project_effect_quad(&screen, points, &projection)) return 0;
        for (unsigned k = 0; k < 4; ++k) {
            tips[i + k] = projection.vertices[k];
            tips[i + k].x += hub.x - 160;
            tips[i + k].y += hub.y - 120;
        }
    }
    tips[RIM] = tips[0];
    size_t count = capacity < FACES ? capacity : FACES;
    for (unsigned i = 0; i < count; ++i) {
        out[i] = (BfmForestDustCard){
            .vertices = {tips[2*i], tips[2*i + 1], hub, tips[2*i + 2]},
            .material = {.page = 32, .count = 4, .translucent = 1}, .bucket = bucket
        };
    }
    return count;
}

size_t bfm_woods_capture_light_project(const BfmWoodsCaptureLight *light,
    const BfmForestCamera *camera, const int32_t eye[3], BfmForestDustCard *out, size_t capacity)
{
    if (!light || !camera || !eye || !out || !capacity) return 0;
    size_t count = 0;
    int16_t rim[RIM][2];
    if (light->show) {
        circle(40, rim);
        count = project_circle(camera, light->center, rim, light->scale_x, light->scale_y,
                               0xe0e0e0, out, capacity);
    }
    int mote_rim = 0;
    for (unsigned i = 0; i < BFM_WOODS_CAPTURE_MOTES && count < capacity; ++i) {
        const BfmWoodsCaptureMote *mote = &light->motes[i];
        if (!mote->remaining) continue;
        if (!mote_rim) {
            circle(24, rim);
            mote_rim = 1;
        }
        int32_t center[3];
        for (unsigned k = 0; k < 3; ++k) center[k] = down(mote->position[k], 65536);
        count += project_circle(camera, center, rim, mote->scale, mote->scale, 0x20e020,
                                out + count, capacity - count);
    }
    return count;
}

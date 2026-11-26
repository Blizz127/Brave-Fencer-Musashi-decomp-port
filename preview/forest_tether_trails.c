/* SPDX-License-Identifier: MIT */
#include "forest_tether_trails.h"
#include "forest_shore_motion.h"
#include <math.h>

enum { UNUSED, BORN, SCATTER, SEEK_BLADE, SEEK_PLAYER, FADE };

static int down(int64_t value, int64_t unit)
{
    return (int)(value / unit - (value % unit < 0));
}

/* Direction lookup uses a base-four mantissa. Keeping that rounding matters
 * when the signed integer velocity crosses a homing speed threshold. */
static uint64_t direction(int delta[3])
{
    uint64_t length = 0;
    for (unsigned k = 0; k < 3; ++k)
        length += (uint64_t)((int64_t)delta[k] * delta[k]);
    if (!length) return 0;
    uint64_t mantissa = length;
    unsigned divisor = 8;
    while (mantissa > 255) { mantissa >>= 2; divisor *= 2; }
    while (mantissa < 64) { mantissa <<= 2; divisor /= 2; }
    int gain = (int)sqrt((double)(UINT32_C(1) << 30) / (double)mantissa);
    for (unsigned k = 0; k < 3; ++k)
        delta[k] = down((int64_t)delta[k] * gain, divisor);
    return length;
}

void bfm_forest_tether_trails_begin(BfmForestTetherTrails *trails,
                             const int16_t target[3], const int16_t blade[3])
{
    if (!trails || !target || !blade) return;
    *trails = (BfmForestTetherTrails){.enabled = 1};
    int offset[3];
    for (unsigned k = 0; k < 3; ++k) offset[k] = target[k] - blade[k];
    offset[1] -= 40;
    direction(offset);
    for (unsigned k = 0; k < 3; ++k)
        trails->origin[k] = (int16_t)(target[k] + down(offset[k], 512) - (k == 1 ? 40 : 0));
    trails->yaw = bfm_forest_shore_bearing(offset[0], offset[2]);
}

/* The effect matrix reflects negative angles through the positive half wave.
 * Reusing the actor table preserves the repeated quarter-wave endpoints. */
static void wave(int angle, int *sine, int *cosine)
{
    const int16_t zero[3] = {0};
    BfmForestTransform turn;
    bfm_forest_actor_root((unsigned)(angle < 0 ? -angle : angle), zero, &turn);
    *sine = angle < 0 ? -turn.rotation[0][2] : turn.rotation[0][2];
    *cosine = turn.rotation[0][0];
}

static int signed_bearing(int x, int z)
{
    int angle = (int)bfm_forest_shore_bearing(x, z);
    return angle > 2048 ? angle - 4096 : angle;
}

static void sample_position(BfmTetherStream *p)
{
    int vx = down(p->velocity[0], 4096), vz = down(p->velocity[2], 4096);
    int sine, cosine;
    wave(signed_bearing(vx, vz), &sine, &cosine);
    int width_axis[3] = {cosine, 0, -sine};
    BfmTetherSample *point = &p->samples[p->head];
    for (unsigned k = 0; k < 3; ++k) {
        int center = down(p->position[k], 65536);
        int width = 9 * width_axis[k];
        point->edge[0][k] = (int16_t)(center + down(width, 4096));
        point->edge[1][k] = (int16_t)(center + down(-width, 4096));
    }
}

static void scatter(BfmTetherStream *p, unsigned rotation, unsigned draw)
{
    int pitch = (int)(draw & 127) * 8 - 128;
    int yaw = (int)((draw >> 8) & 127) * 16 - 1024;
    int sp, cp, sy, cy, sb, cb;
    wave(pitch, &sp, &cp);
    wave(yaw, &sy, &cy);
    wave(rotation > 2048 ? (int)rotation - 4096 : (int)rotation, &sb, &cb);
    int x = down(sy * cp, 4096), z = down(cy * cp, 4096);
    int axis[3] = {down(cb*x + sb*z, 4096), -sp, down(cb*z - sb*x, 4096)};
    for (unsigned k = 0; k < 3; ++k)
        p->velocity[k] = down(11 * axis[k], 4096) * 65536;
    p->phase = SCATTER;
    p->ticks = 9;
    p->light = 240;
    sample_position(p);
}

static void fade(BfmTetherStream *p)
{
    if (p->links <= 1) {
        p->phase = UNUSED;
        p->links = 0;
        return;
    }
    p->phase = FADE;
    --p->links;
    p->light -= 15;
}

static void step(BfmTetherStream *p, const int16_t blade[3], const int16_t player[3])
{
    if (p->phase == FADE) { fade(p); return; }
    if (p->phase == SCATTER) {
        if (!--p->ticks || !blade) p->phase = SEEK_BLADE;
    } else {
        if (p->phase == SEEK_BLADE && !blade) {
            p->phase = SEEK_PLAYER;
            p->ticks = 64;
        }
        int returning = p->phase == SEEK_PLAYER;
        const int16_t *goal = returning ? player : blade;
        int delta[3], velocity[3];
        for (unsigned k = 0; k < 3; ++k)
            delta[k] = goal[k] - down(p->position[k], 65536);
        uint64_t d2 = direction(delta);
        int near = returning ? 44 : 36;
        if (d2 < (unsigned)(near * near) || (returning && !--p->ticks)) {
            fade(p);
            return;
        }
        int gain = returning || d2 < 80 * 80 ? 64 : (d2 < 160 * 160 ? 32 : 16);
        for (unsigned k = 0; k < 3; ++k) {
            p->velocity[k] += delta[k] * gain;
            velocity[k] = down(p->velocity[k], 65536);
        }
        int cap = returning ? 64 : 32;
        if (direction(velocity) > (unsigned)(cap * cap)) {
            for (unsigned k = 0; k < 3; ++k)
                p->velocity[k] = velocity[k] * (returning ? 1024 : 512);
        }
    }
    for (unsigned k = 0; k < 3; ++k) p->position[k] += p->velocity[k];
    p->head = (p->head + 1) % BFM_TETHER_SAMPLES;
    if (p->links + 1 < BFM_TETHER_SAMPLES) ++p->links;
    sample_position(p);
}

void bfm_forest_tether_trails_tick(BfmForestTetherTrails *trails, int driven, unsigned meter,
                            const int16_t blade[3], const int16_t player[3],
                            BfmTetherRandom random, void *context)
{
    if (!trails || !player || !random || !trails->enabled) return;
    BfmTetherStream *free_slot = NULL;
    unsigned active = 0;
    for (unsigned i = 0; i < BFM_TETHER_STREAMS; ++i) {
        BfmTetherStream *p = &trails->streams[i];
        if (p->phase == BORN) scatter(p, trails->yaw, random(context));
        else if (p->phase != UNUSED) step(p, blade, player);
        if (p->phase != UNUSED) ++active;
        else if (!free_slot) free_slot = p;
    }
    if (!blade && !active) trails->enabled = 0;
    if (!blade || !driven) return;
    if (trails->cooldown && meter <= trails->last_meter) {
        --trails->cooldown;
        return;
    }
    trails->last_meter = meter;
    if (free_slot) {
        *free_slot = (BfmTetherStream){.phase = BORN};
        for (unsigned k = 0; k < 3; ++k)
            free_slot->position[k] = (int64_t)trails->origin[k] * 65536;
        trails->cooldown = 16;
    }
}

static unsigned green(unsigned g)
{
    return ((g / 8) << 16) | (g << 8) | (g / 8);
}

static void section(const BfmTetherSample *sample, unsigned side, int16_t edge[3], int16_t center[3])
{
    for (unsigned k = 0; k < 3; ++k) {
        int mid = down(sample->edge[0][k] + sample->edge[1][k], 2);
        edge[k] = sample->edge[side][k];
        center[k] = (int16_t)mid;
    }
}

size_t bfm_forest_tether_trails_project(const BfmForestTetherTrails *trails,
                  const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity)
{
    if (!trails || !camera || !out) return 0;
    size_t written = 0;
    for (unsigned i = 0; i < BFM_TETHER_STREAMS; ++i) {
        const BfmTetherStream *p = &trails->streams[i];
        unsigned index = p->head;
        for (unsigned link = 0; link < p->links; ++link) {
            unsigned previous = (index + BFM_TETHER_SAMPLES - 1) % BFM_TETHER_SAMPLES;
            unsigned head_light = p->light - link * 15;
            for (unsigned side = 0; side < 2; ++side) {
                int16_t corners[4][3];
                section(&p->samples[index], side, corners[side], corners[1 - side]);
                section(&p->samples[previous], side, corners[2 + side], corners[3 - side]);
                BfmForestDustCard quad = {
                    .material = {.page = 32, .count = 4, .translucent = 1}
                };
                quad.vertices[1-side].rgb = green(head_light);
                quad.vertices[3-side].rgb = green(head_light - 15);
                if (written == capacity) return written;
                if (bfm_forest_project_effect_quad(camera, corners, &quad)) out[written++] = quad;
            }
            index = previous;
        }
    }
    return written;
}

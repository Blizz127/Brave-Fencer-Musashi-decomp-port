/* SPDX-License-Identifier: MIT */
#include "woods_shrink_ribbon.h"
#include <math.h>

enum { STRANDS = 4, SEGMENTS = 11, HISTORY = 20, ONE = 4096 };

typedef struct { int16_t x, y, z; } RibbonPoint;

static int floor_q12(int value)
{
    return value / ONE - (value % ONE < 0);
}

static RibbonPoint center_at(unsigned age)
{
    RibbonPoint center = {0, -24, (int16_t)(-15 - 6 * (int)age)};
    if (age) {
        const double radians = 6.28318530717958647693 / ONE;
        unsigned tick = age - 1;
        double orbit = ((tick * 227) % ONE) * radians;
        double pulse = ((tick * 56) % ONE) * radians;
        int radius = (int)lround(ONE * sin(pulse)) / 128;
        center.x += (int16_t)(radius * (int)lround(ONE * cos(orbit)) / ONE);
        center.y += (int16_t)(radius * (int)lround(ONE * sin(orbit)) / ONE);
    }
    return center;
}

static BfmForestTransform strand_root(const BfmWoodsShrinkShot *shot, unsigned strand)
{
    /* The roll uses the same packed rotation wave as the body. Rotate its
     * first two columns around local Z before tapering each segment. */
    const int16_t origin[3] = {0};
    BfmForestTransform turn, root = shot->root;
    bfm_forest_actor_root(strand * 1024 + (shot->age - 1) * 113, origin, &turn);
    int sine = turn.rotation[0][2], cosine = turn.rotation[0][0];
    for (unsigned row = 0; row < 3; ++row) {
        int x = shot->root.rotation[row][0], y = shot->root.rotation[row][1];
        root.rotation[row][0] = (int16_t)floor_q12(x * cosine + y * sine);
        root.rotation[row][1] = (int16_t)floor_q12(y * cosine - x * sine);
    }
    return root;
}

static uint32_t ribbon_color(unsigned strand, unsigned fade)
{
    unsigned blue = strand & 1;
    unsigned r = blue ? 64 : 255, g = blue ? 64 : 240, b = blue ? 255 : 64;
    return (r > fade ? r - fade : 0) | ((g > fade ? g - fade : 0) << 8) |
           ((b > fade ? b - fade : 0) << 16);
}

size_t bfm_woods_shrink_ribbon_project(const BfmWoodsShrinkShot *shot,
    const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity)
{
    if (!shot || !shot->active || !shot->age || shot->age > BFM_WOODS_SHRINK_SHOT_TICKS ||
        !camera || !out || !capacity) return 0;

    RibbonPoint trail[HISTORY + 1];
    for (unsigned i = 0; i <= HISTORY; ++i)
        trail[i] = center_at(shot->age > i ? shot->age - i : 0);

    size_t count = 0;
    for (unsigned strand = 0; strand < STRANDS; ++strand) {
        BfmForestTransform root = strand_root(shot, strand);
        unsigned fade = 0;
        for (unsigned segment = 0; segment < SEGMENTS; ++segment) {
            /* The head meets the previous sample. The middle spans every
             * other sample, and the final span picks up the oldest pair. */
            unsigned first = segment ? 2 * segment - 1 : 0;
            unsigned last = 2 * segment + 1;
            if (segment == SEGMENTS - 1) { first = HISTORY - 2; last = HISTORY; }
            RibbonPoint a = trail[first], b = trail[last];
            const int16_t corners[4][3] = {
                {a.x - 8, a.y, a.z}, {a.x + 8, a.y, a.z},
                {b.x - 8, b.y, b.z}, {b.x + 8, b.y, b.z}
            };
            int scale = ONE - (int)segment * 256;
            BfmForestTransform tapered = root;
            for (unsigned row = 0; row < 3; ++row)
                for (unsigned col = 0; col < 3; ++col)
                    tapered.rotation[row][col] = (int16_t)floor_q12(root.rotation[row][col] * scale);
            BfmForestCamera local;
            BfmForestDustCard card = {.material = {.page = 32, .count = 4, .translucent = 1}};
            uint32_t head = ribbon_color(strand, fade);
            if (segment) fade += segment == SEGMENTS - 1 ? 10 : 12;
            if (!bfm_forest_effect_camera(camera, &tapered, &local) ||
                !bfm_forest_project_effect_quad(&local, corners, &card)) continue;
            card.vertices[0].rgb = card.vertices[1].rgb = head;
            card.vertices[2].rgb = card.vertices[3].rgb = ribbon_color(strand, fade);
            out[count++] = card;
            if (count == capacity) return count;
        }
    }
    return count;
}

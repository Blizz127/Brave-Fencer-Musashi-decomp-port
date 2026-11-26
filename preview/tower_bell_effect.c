/* SPDX-License-Identifier: MIT */
#include "tower_bell_effect.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_tower_bell_effect_load(const uint8_t *scene, size_t size, BfmTowerBellEffect *effect)
{
    /* Two three-spoke arcs bound the outer and inner edges of each light. */
    const size_t at = UINT32_C(0x8018a998) - UINT32_C(0x80128158);
    if (!scene || !effect || at > size || size - at < 16) return 0;
    BfmTowerBellEffect loaded = {0};
    for (unsigned ring = 0; ring < 2; ++ring) {
        loaded.radius[ring] = s16le(scene + at + 12 + 2*ring);
        if (loaded.radius[ring] <= 0) return 0;
        for (unsigned j = 0; j < 3; ++j)
            loaded.offsets[ring][j] = s16le(scene + at + 2*(3*ring + j));
    }
    *effect = loaded;
    return 1;
}

static int floor_q12(int64_t v)
{
    return (int)(v / 4096 - (v % 4096 < 0));
}

void bfm_tower_bell_effect_start(BfmTowerBellEffect *effect, BfmGameplayRun *run)
{
    if (!effect || !run || !effect->radius[0] || effect->started) return;
    for (unsigned i = 0; i < BFM_TOWER_BELL_RAYS; ++i) {
        int yaw = bfm_gameplay_random(run) & 4095;
        int height = -384 - (bfm_gameplay_random(run) & 1023);
        for (unsigned ring = 0; ring < 2; ++ring) {
            for (unsigned j = 0; j < 3; ++j) {
                BfmForestTransform rotation;
                unsigned angle = (unsigned)(yaw - effect->offsets[ring][j] - 1024) & 4095;
                bfm_forest_actor_root(angle, (const int16_t[3]){0}, &rotation);
                int16_t *point = effect->corners[i][3*ring + j];
                int r = effect->radius[ring];
                point[0] = (int16_t)floor_q12((int64_t)r * rotation.rotation[0][0]);
                point[1] = (int16_t)height;
                point[2] = (int16_t)floor_q12((int64_t)r * rotation.rotation[2][0]);
            }
        }
    }
    effect->started = 1;
}

static int ray_cards(const int16_t corners[6][3], unsigned level,
                     const BfmForestCamera *camera, BfmForestDustCard pair[2])
{
    /* Each half runs from a dark edge to the same bright centerline. Both
     * halves share the ordering depth averaged over all six projected points. */
    int depth_sum = 0;
    for (unsigned i = 0; i < 6; ++i) {
        int64_t dot = 0;
        for (unsigned j = 0; j < 3; ++j)
            dot += (int64_t)camera->rotation[2][j] * corners[i][j];
        int64_t depth = (int64_t)camera->translation[2] + floor_q12(dot);
        if (depth <= 0 || depth >= 16384) return 0;
        depth_sum += (int)depth / 4;
    }
    unsigned depth = (unsigned)depth_sum / 6;
    if (!depth) return 0;
    unsigned bright = level | level << 8 | ((7*level)/8) << 16;
    unsigned faint = (bright & 0xfcfcfcu) >> 2;
    for (unsigned side = 0; side < 2; ++side) {
        unsigned edge = 2*side;
        const unsigned index[4] = {edge, 3+edge, 1, 4};
        int16_t quad[4][3];
        for (unsigned i = 0; i < 4; ++i)
            for (unsigned j = 0; j < 3; ++j)
                quad[i][j] = corners[index[i]][j];
        BfmForestDustCard card = {0};
        if (!bfm_forest_project_effect_quad(camera, quad, &card)) return 0;
        card.bucket = depth;
        card.material = (BfmForestRasterMaterial){.count=4, .translucent=1, .page=32};
        card.vertices[2].rgb = bright;
        card.vertices[3].rgb = faint;
        pair[side] = card;
    }
    return 1;
}

size_t bfm_tower_bell_effect_cards(const BfmTowerBellEffect *effect, unsigned age,
                                   const BfmForestCamera *camera, BfmForestDustCard *out,
                                   size_t capacity)
{
    if (!effect || !effect->started || !camera || !out || !age || age > 256) return 0;
    unsigned lit = (age - 1)/4;
    if (lit > BFM_TOWER_BELL_RAYS) lit = BFM_TOWER_BELL_RAYS;
    size_t count = 0;
    for (unsigned i = 0; i < lit && capacity - count >= 2; ++i) {
        unsigned level = 31 + age - 4*(i+1);
        if (level > 255) level = 255;
        if (ray_cards(effect->corners[i], level, camera, out + count))
            count += 2;
    }
    return count;
}

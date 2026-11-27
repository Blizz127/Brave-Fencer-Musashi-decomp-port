/* SPDX-License-Identifier: MIT */
#include "woods_shrink_light.h"
#include "forest_bullet_art.h"
#include "woods_capture_light.h"
#include <string.h>

enum { SHRINK = 13, UNIT = 4096, CONTRACT_TICKS = 6, FADE_TICKS = 16 };

void bfm_woods_shrink_light_tick(BfmWoodsShrinkLight *light, const BfmWoodsShrinkShot *shot,
    const int32_t eye[3], unsigned field_tick, int emitted, unsigned equipped)
{
    if (!light) return;
    if (equipped != SHRINK) {
        light->scale = 0;
        return;
    }
    if (emitted && shot && eye) {
        /* The shot frame is already positioned at the muzzle. Only this
         * stationary flash takes the extra camera-facing displacement. */
        BfmForestTransform biased;
        bfm_forest_glow_facing(eye, shot->root.translation, 6, &biased);
        memcpy(light->center, biased.translation, sizeof light->center);
        light->age = 0;
    } else {
        if (!light->scale) return;
        ++light->age;
    }

    if (light->age < CONTRACT_TICKS) {
        light->scale = 3 * UNIT - (int)light->age * 1536;
    } else {
        unsigned fade = light->age - CONTRACT_TICKS;
        light->scale = fade < FADE_TICKS ? UNIT - 256 * (int)fade : 0;
        if (fade && (field_tick & 1u)) light->scale /= 2;
    }
    light->color = (field_tick & 3u) ? UINT32_C(0xff0000) : UINT32_C(0x00ffff);
}

size_t bfm_woods_shrink_light_project(const BfmWoodsShrinkLight *light,
    const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity)
{
    if (!light || !light->scale) return 0;
    return bfm_woods_radial_light_project(camera, light->center, 96, light->scale,
                                          light->color, out, capacity);
}

void bfm_woods_shrink_player_lamps(unsigned field_tick, BfmBedroomLight lamps[3])
{
    if (!lamps) return;
    const int16_t origin[3] = {0};
    BfmForestTransform angle;
    bfm_forest_actor_root((field_tick & 7u) * 512u, origin, &angle);
    int16_t sine = angle.rotation[0][2], cosine = angle.rotation[0][0];
    lamps[0] = (BfmBedroomLight){.axis = {sine, 0, cosine}, .rgb = {255, 0, 0}};
    lamps[1] = (BfmBedroomLight){.axis = {sine, cosine, 0}, .rgb = {0, 0, 255}};
    lamps[2] = (BfmBedroomLight){.axis = {0, sine, cosine}, .rgb = {255, 255, 255}};
}

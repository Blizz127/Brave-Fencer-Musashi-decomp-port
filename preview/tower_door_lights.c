/* SPDX-License-Identifier: MIT */
#include "tower_door_lights.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>

static int down(int64_t value, int divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

int bfm_tower_door_lights_load(BfmTowerDoorLights *lights, const uint8_t *scene, size_t bytes)
{
    const size_t sprite = UINT32_C(0x8018a220) - UINT32_C(0x80128158);
    if (!lights || !scene || sprite > bytes || bytes - sprite < 12) return 0;
    const uint8_t *desc = scene + sprite;
    unsigned u = 4 * u16le(desc + 4), v = 128 + u16le(desc + 6);
    if (desc[0] != 255 || !desc[2] || !desc[3] ||
        u + desc[2] > 256 || v + desc[3] > 256) return 0;
    BfmTowerDoorLights parsed = {.width=desc[2], .height=desc[3], .u=(uint8_t)u,
                                  .v=(uint8_t)v, .pivot_x=s16le(desc+8), .pivot_y=s16le(desc+10)};
    /* Adjacent strips share the integral rim of the fixed lamp. */
    for (unsigned i = 0; i < 12; ++i) {
        unsigned acute = i <= 3 ? i : i <= 6 ? 6-i : i <= 9 ? i-6 : 12-i;
        BfmForestTransform turn;
        bfm_forest_actor_root(341*acute, (const int16_t[3]){0}, &turn);
        int x = down(80*turn.rotation[0][2], 4096);
        int y = down(80*turn.rotation[0][0], 4096);
        parsed.rim[i][0] = (int16_t)(i > 6 ? -x : x);
        parsed.rim[i][1] = (int16_t)(i > 3 && i < 9 ? -y : y);
    }
    *lights = parsed;
    return 1;
}

static size_t fan(const BfmTowerDoorLights *lights, const BfmForestCamera *camera,
                  const int16_t center[3], int yaw, BfmForestDustCard *out)
{
    BfmForestTransform frame;
    BfmForestCamera local;
    bfm_forest_actor_pitch_yaw(4096-256, (unsigned)yaw & 4095, center, &frame);
    if (!bfm_forest_effect_camera(camera, &frame, &local) || local.translation[2] <= 0)
        return 0;
    unsigned bucket = (unsigned)local.translation[2]/4 + 1;
    if (bucket >= 4096) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < 12; i += 2) {
        int16_t quad[4][3] = {{0}};
        for (unsigned j = 0; j < 3; ++j) {
            const int16_t *v = lights->rim[(i+j)%12];
            unsigned at = j == 2 ? 3 : j;
            quad[at][0] = v[0];
            quad[at][1] = v[1];
        }
        BfmForestDustCard card = {0};
        if (!bfm_forest_project_effect_quad(&local, quad, &card)) continue;
        card.bucket = bucket;
        card.material = (BfmForestRasterMaterial){.count=4, .translucent=1, .page=32};
        card.vertices[2].rgb = UINT32_C(0xc0c0e0);
        out[count++] = card;
    }
    return count;
}

static int star(const BfmTowerDoorLights *lights, const BfmForestCamera *camera,
                const int16_t center[3], BfmForestDustCard *out)
{
    /* Form a separate line of sight to each light before applying its roll. */
    int d[3];
    for (unsigned j = 0; j < 3; ++j) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k)
            dot += (int64_t)camera->rotation[k][j] * camera->translation[k];
        d[j] = -down(dot,4096) - center[j];
    }
    unsigned yaw = bfm_forest_shore_bearing(d[0],d[2]);
    unsigned pitch = (0u - bfm_forest_shore_bearing(d[1],(int)hypot(d[0],d[2]))) & 4095;
    BfmForestTransform face, roll;
    bfm_forest_actor_pitch_yaw(pitch, yaw, center, &face);
    bfm_forest_actor_root((4*yaw)&4095, (const int16_t[3]){0}, &roll);
    BfmForestCamera local;
    if (!bfm_forest_effect_camera(camera, &face, &local)) return 0;
    int16_t quad[4][3] = {{0}};
    BfmForestDustCard card = {0};
    int depth_sum = 0;
    for (unsigned i = 0; i < 4; ++i) {
        int x = lights->pivot_x + (int)(i%2)*lights->width;
        int y = lights->pivot_y + (int)(i/2)*lights->height;
        quad[i][0] = (int16_t)down(x*roll.rotation[0][0]-y*roll.rotation[0][2],4096);
        quad[i][1] = (int16_t)down(x*roll.rotation[0][2]+y*roll.rotation[0][0],4096);
        depth_sum += local.translation[2] + down((int64_t)local.rotation[2][0]*quad[i][0] +
                                               (int64_t)local.rotation[2][1]*quad[i][1], 4096);
        card.vertices[i].u = (uint8_t)(lights->u + (i%2)*(lights->width-1));
        card.vertices[i].v = (uint8_t)(lights->v + (i/2)*(lights->height-1));
        card.vertices[i].rgb = UINT32_C(0x808080);
    }
    if (!bfm_forest_project_effect_quad(&local, quad, &card)) return 0;
    card.bucket = (unsigned)depth_sum/16 + 1;
    if (card.bucket >= 4096) return 0;
    card.material = (BfmForestRasterMaterial){
        .count=4, .textured=1, .translucent=1, .page=58, .clut=(333u<<6) | (352u/16)
    };
    *out = card;
    return 1;
}

size_t bfm_tower_door_light_cards(const BfmTowerDoorLights *lights, const BfmForestCamera *camera,
                                 BfmForestDustCard out[BFM_TOWER_DOOR_CARDS])
{
    if (!lights || !camera || !camera->projection || !out) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < 2; ++i) {
        const int16_t center[3] = {i ? -96 : 0, -1664, 240};
        count += fan(lights, camera, center, i ? 416 : -512, out + count);
        if (star(lights, camera, center, out + count)) ++count;
    }
    return count;
}

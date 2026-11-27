/* SPDX-License-Identifier: MIT */
#include "woods_resident_glow_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>
#include <stdlib.h>

enum { PARTS = 9, RIM = 12, ONE = 4096 };

struct BfmWoodsResidentGlowArt {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
    int16_t rim[RIM][2];
};

static int down(int value, int unit)
{
    return value / unit - (value % unit < 0);
}

static const uint8_t *mounted(const uint8_t *scene, size_t size, uint32_t address, size_t length)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || address - base > size || length > size - (address - base)) return NULL;
    return scene + (address - base);
}

BfmWoodsResidentGlowArt *bfm_woods_resident_glow_art_load(const uint8_t *scene, size_t size)
{
    const uint8_t *resource = mounted(scene, size, UINT32_C(0x8018b24c), 8);
    const uint8_t *joints = mounted(scene, size, UINT32_C(0x801d5ffc), PARTS * 12);
    if (!resource || !joints || size > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmWoodsResidentGlowArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    unsigned count;
    art->mesh = musashi_forest_player_mesh_from_table(scene, size, UINT32_C(0x80128158),
                                                      u32le(resource + 4), &count);
    if (!art->mesh || count != PARTS) goto fail;

    /* This model keeps full-width local translations and angles. Feed its
     * rotations through the common rigid-pose reader, then retain the full
     * translations instead of narrowing them to the compact actor format. */
    uint8_t angles[PARTS * 8] = {0};
    for (unsigned i = 0; i < PARTS; ++i) {
        const uint8_t *joint = joints + 12*i;
        unsigned y = u16le(joint + 8) & 4095, z = u16le(joint + 10) & 4095;
        angles[8*i] = (uint8_t)((y >> 8) | ((z >> 4) & 0xf0));
        angles[8*i+1] = (uint8_t)y;
        angles[8*i+2] = (uint8_t)z;
        angles[8*i+6] = joint[6];
        angles[8*i+7] = joint[7];
    }
    if (!bfm_forest_actor_pose_decode(angles, sizeof angles, PARTS, &art->pose)) goto fail;
    for (unsigned i = 0; i < PARTS; ++i)
        for (unsigned j = 0; j < 3; ++j)
            art->pose.local[i].translation[j] = s16le(joints + 12*i + 2*j);

    /* The projected light has four sampled points in its first quadrant.
     * Opposite points reflect those same integer coordinates. */
    const double radians = 6.28318530717958647693 / ONE;
    for (unsigned i = 0; i < RIM; ++i) {
        unsigned acute = i <= 3 ? i : i <= 6 ? 6-i : i <= 9 ? i-6 : 12-i;
        double angle = radians * 341 * acute;
        int x = (int)(56 * lround(ONE * sin(angle)) / ONE);
        int z = (int)(56 * lround(ONE * cos(angle)) / ONE);
        art->rim[i][0] = (int16_t)(i > 6 ? -x : x);
        art->rim[i][1] = (int16_t)(i > 3 && i < 9 ? -z : z);
    }
    return art;
fail:
    bfm_woods_resident_glow_art_free(art);
    return NULL;
}

void bfm_woods_resident_glow_art_free(BfmWoodsResidentGlowArt *art)
{
    if (art) musashi_forest_player_mesh_free(art->mesh);
    free(art);
}

static size_t project_glow(const BfmWoodsResidentGlowArt *art, const BfmBedroomView *view,
                           const int16_t root[3], int scale, BfmForestDustCard cards[RIM/2])
{
    int hub[3];
    if (!bfm_bedroom_project(view, root, hub) || hub[2] <= view->focal/2 || hub[2]/4+1 >= 4096)
        return 0;
    BfmForestRasterVertex tips[RIM] = {0};
    for (unsigned i = 0; i < RIM; ++i) {
        int x = root[0] + down(scale * art->rim[i][0], ONE);
        int z = root[2] + down(scale * art->rim[i][1], ONE);
        if (x < INT16_MIN || x > INT16_MAX || z < INT16_MIN || z > INT16_MAX) return 0;
        int16_t world[3] = {(int16_t)x, root[1], (int16_t)z};
        int screen[3];
        if (!bfm_bedroom_project(view, world, screen) || screen[2] <= view->focal/2) return 0;
        tips[i] = (BfmForestRasterVertex){.x=screen[0], .y=screen[1]};
    }
    const BfmForestRasterVertex center = {.x=hub[0], .y=hub[1], .rgb=UINT32_C(0x80e080)};
    for (unsigned i = 0; i < RIM; i += 2) {
        cards[i/2] = (BfmForestDustCard){
            .vertices={tips[i], tips[i+1], center, tips[(i+2)%RIM]},
            .material={.count=4, .page=32, .translucent=1}, .bucket=(unsigned)(hub[2]/4+1)
        };
    }
    return RIM/2;
}

int bfm_woods_resident_glow_art_add(const BfmWoodsResidentGlowArt *art, BfmBedroomFigure *figure,
    const BfmBedroomView *view, const int16_t root[3], unsigned yaw, unsigned light_pitch,
    unsigned light_yaw, int flash, int lights_active)
{
    if (!art || !figure || !view || !root) return 0;
    BfmForestTransform facing, source;
    bfm_forest_actor_root(yaw, root, &facing);
    bfm_forest_actor_pitch_yaw(light_pitch, light_yaw, (const int16_t[3]){0}, &source);
    BfmBedroomLight lights[3];
    for (unsigned i = 0; i < 3; ++i) {
        lights[i] = (BfmBedroomLight){.rgb={0, 0, 0}};
        if (lights_active) {
            lights[i].rgb[0] = i == 2 ? 0 : 255;
            lights[i].rgb[1] = 128;
            lights[i].rgb[2] = 255;
        }
        for (unsigned j = 0; j < 3; ++j)
            lights[i].axis[j] = (int16_t)down(source.rotation[j][2] * 256, ONE);
    }
    const BfmForestActor crystal = {.mesh=art->mesh, .pose=&art->pose, .root=&facing,
                                    .visible_parts=(1u<<PARTS)-1};
    if (!bfm_bedroom_figure_add_lit_translucent(figure, view, &crystal, lights, flash ? 1 : 0)) return 0;
    BfmForestDustCard glow[RIM/2];
    size_t count = project_glow(art, view, root, flash ? 4915 : ONE, glow);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, glow, count);
}

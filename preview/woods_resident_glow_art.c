/* SPDX-License-Identifier: MIT */
#include "woods_resident_glow_art.h"
#include "woods_fusion_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { PARTS = 9, RIM = 12, ONE = 4096 };

struct BfmWoodsResidentGlowArt {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
    uint8_t joints[PARTS * 12];
    int16_t ground_rim[RIM][2], white_rim[RIM][2];
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

static void make_rim(int radius, int16_t rim[RIM][2])
{
    const double radians = 6.28318530717958647693 / ONE;
    for (unsigned i = 0; i < RIM; ++i) {
        unsigned acute = i <= 3 ? i : i <= 6 ? 6-i : i <= 9 ? i-6 : 12-i;
        double angle = radians * 341 * acute;
        int x = (int)(radius * lround(ONE * sin(angle)) / ONE);
        int z = (int)(radius * lround(ONE * cos(angle)) / ONE);
        rim[i][0] = (int16_t)(i > 6 ? -x : x);
        rim[i][1] = (int16_t)(i > 3 && i < 9 ? -z : z);
    }
}

BfmWoodsResidentGlowArt *bfm_bincho_glow_art_load(const uint8_t *scene, size_t size,
    uint32_t profile_address, uint32_t joints_address)
{
    const uint8_t *resource = mounted(scene, size, profile_address, 8);
    const uint8_t *joints = mounted(scene, size, joints_address, PARTS * 12);
    if (!resource || !joints || size > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmWoodsResidentGlowArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    unsigned count;
    art->mesh = musashi_forest_player_mesh_from_table(scene, size, UINT32_C(0x80128158),
                                                      u32le(resource + 4), &count);
    if (!art->mesh || count != PARTS) goto fail;

    memcpy(art->joints, joints, sizeof art->joints);
    if (!bfm_forest_actor_pose_decode_full(joints, sizeof art->joints, PARTS, &art->pose)) goto fail;

    /* The projected light has four sampled points in its first quadrant.
     * Opposite points reflect those same integer coordinates. */
    make_rim(56, art->ground_rim);
    make_rim(256, art->white_rim);
    return art;
fail:
    bfm_woods_resident_glow_art_free(art);
    return NULL;
}

BfmWoodsResidentGlowArt *bfm_woods_resident_glow_art_load(const uint8_t *scene, size_t size)
{
    return bfm_bincho_glow_art_load(scene, size, UINT32_C(0x8018b24c), UINT32_C(0x801d5ffc));
}

void bfm_woods_resident_glow_art_free(BfmWoodsResidentGlowArt *art)
{
    if (art) musashi_forest_player_mesh_free(art->mesh);
    free(art);
}

static size_t project_glow(const int16_t rim[RIM][2], const BfmBedroomView *view,
                           const int16_t root[3], unsigned plane_axis, int scale,
                           uint32_t hub_rgb, uint32_t rim_rgb, BfmForestDustCard cards[RIM/2])
{
    int hub[3];
    if (!bfm_bedroom_project(view, root, hub) || hub[2] <= view->focal/2 || hub[2]/4+1 >= 4096)
        return 0;
    BfmForestRasterVertex tips[RIM];
    for (unsigned i = 0; i < RIM; ++i) {
        int16_t p[3] = {root[0], root[1], root[2]};
        int x = root[0] + down(scale * rim[i][0], ONE);
        int v = root[plane_axis] + down(scale * rim[i][1], ONE);
        if (x < INT16_MIN || x > INT16_MAX || v < INT16_MIN || v > INT16_MAX) return 0;
        p[0] = (int16_t)x;
        p[plane_axis] = (int16_t)v;
        int screen[3];
        int visible = bfm_bedroom_project(view, p, screen);
        if (plane_axis == 2 && (!visible || screen[2] <= view->focal/2)) return 0;
        tips[i] = (BfmForestRasterVertex){.x=screen[0], .y=screen[1], .rgb=rim_rgb};
    }
    const BfmForestRasterVertex center = {.x=hub[0], .y=hub[1], .rgb=hub_rgb};
    for (unsigned i = 0; i < RIM; i += 2) {
        cards[i/2] = (BfmForestDustCard){
            .vertices={tips[i], tips[i+1], center, tips[(i+2)%RIM]},
            .material={.count=4, .page=32, .translucent=1}, .bucket=(unsigned)(hub[2]/4+1)
        };
    }
    return RIM/2;
}

/* The white light faces the eye at birth. Its position and orientation stay
 * fixed while the field changes its size, even if the camera moves. */
int bfm_woods_resident_release_glow_add(const BfmWoodsResidentGlowArt *art, BfmBedroomFigure *figure,
    const BfmBedroomView *view, const BfmForestTransform *facing, unsigned scale)
{
    if (!art || !figure || !facing || scale > INT16_MAX) return 0;
    BfmForestCamera camera, local;
    BfmForestTransform scaled = *facing;
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned column = 0; column < 3; ++column)
            scaled.rotation[row][column] = (int16_t)down(scaled.rotation[row][column] * (int)scale, ONE);
    if (!bfm_woods_effect_camera(view, &camera) || !bfm_forest_effect_camera(&camera, &scaled, &local)) return 0;
    BfmBedroomView projection = {.focal=local.projection};
    for (unsigned row = 0; row < 3; ++row) {
        projection.translation[row] = local.translation[row];
        for (unsigned column = 0; column < 3; ++column)
            projection.axes[row][column] = local.rotation[row][column];
    }
    BfmForestDustCard cards[RIM/2];
    size_t n = project_glow(art->white_rim, &projection, (const int16_t[3]){0}, 1,
                            ONE, UINT32_C(0xffffff), UINT32_C(0x081808), cards);
    return !n || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, n);
}

/* The crystal breaks its own nine posed meshes apart. Its central joint
 * retains its height and orientation. The surrounding eight shards move
 * away from a point 48 units above the actor and turn on all three axes. */
static int shatter_pose(const BfmWoodsResidentGlowArt *art, unsigned ticks, BfmForestPose *pose)
{
    if (ticks > 40) return 0;
    uint8_t frame[sizeof art->joints];
    memcpy(frame, art->joints, sizeof frame);
    for (unsigned i = 0; i < PARTS; ++i) {
        uint8_t *joint = frame + 12*i;
        for (unsigned k = 0; k < (i ? 6u : 1u); ++k) {
            int initial = s16le(joint + 2*k);
            int delta = k < 3 ? down(initial + (k == 1 ? 48 : 0), 2) : (64 << (k - 3));
            uint16_t v = (uint16_t)(initial + (int)ticks * delta);
            joint[2*k] = (uint8_t)v;
            joint[2*k+1] = (uint8_t)(v >> 8);
        }
    }
    return bfm_forest_actor_pose_decode_full(frame, sizeof frame, PARTS, pose);
}

int bfm_woods_resident_glow_art_add(const BfmWoodsResidentGlowArt *art, BfmBedroomFigure *figure,
    const BfmBedroomView *view, const int16_t root[3], unsigned yaw, unsigned light_pitch,
    unsigned light_yaw, int flash, int lights_active, unsigned burst_updates, int floor_glow)
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
    BfmForestPose burst;
    if (burst_updates && !shatter_pose(art, burst_updates, &burst)) return 0;
    const BfmForestActor crystal = {.mesh=art->mesh, .pose=burst_updates ? &burst : &art->pose,
                                    .root=&facing,
                                    .visible_parts=(1u<<PARTS)-1};
    if (!bfm_bedroom_figure_add_lit_translucent(figure, view, &crystal, lights, flash ? 1 : 0)) return 0;
    if (!floor_glow) return 1;
    BfmForestDustCard glow[RIM/2];
    size_t count = project_glow(art->ground_rim, view, root, 2, flash ? 4915 : ONE,
                                UINT32_C(0x80e080), 0, glow);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, glow, count);
}

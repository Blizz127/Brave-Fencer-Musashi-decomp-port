/* SPDX-License-Identifier: MIT */
#include "tower_interior_bell.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

static const uint32_t scene_origin = UINT32_C(0x80128158);

struct BfmTowerInteriorBell {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
    BfmForestTransform root;
    int16_t position[3];
    uint16_t yaw;
    unsigned speed, acceleration;
};

static const uint8_t *mounted(const uint8_t *scene, size_t size, uint32_t address, size_t len)
{
    if (!scene || address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= size && len <= size - offset ? scene + offset : NULL;
}

BfmTowerInteriorBell *bfm_tower_interior_bell_open(const uint8_t *scene, size_t size)
{
    const uint8_t *records = mounted(scene, size, UINT32_C(0x801a3060) + 0xb4, 4);
    if (!records) return NULL;
    const uint8_t *spawn = mounted(scene, size, u32le(records), 20);
    const uint8_t *swing = mounted(scene, size, UINT32_C(0x8018a9d0), 4);
    if (!spawn || u16le(spawn + 6) != 22 || !swing ||
        s16le(swing) <= 0 || s16le(swing + 2) <= 0 || s16le(swing) % s16le(swing + 2))
        return NULL;
    BfmTowerInteriorBell *bell = calloc(1, sizeof *bell);
    if (!bell) return NULL;
    unsigned parts = 0;
    bell->mesh = musashi_forest_player_mesh_from_table(scene, size, scene_origin,
                                                       UINT32_C(0x801be010), &parts);
    if (!bell->mesh || parts != 1) {
        bfm_tower_interior_bell_free(bell);
        return NULL;
    }
    for (unsigned i = 0; i < 3; ++i) bell->position[i] = s16le(spawn + 2*i);
    bell->yaw = u16le(spawn + 12);
    bell->speed = u16le(swing);
    bell->acceleration = u16le(swing + 2);
    /* The idle clip binds the resident zero-translation, zero-rotation pose. */
    bfm_forest_actor_root(0, (const int16_t[3]){0}, &bell->pose.local[0]);
    return bell;
}

void bfm_tower_interior_bell_free(BfmTowerInteriorBell *bell)
{
    if (bell) {
        musashi_forest_player_mesh_free(bell->mesh);
        free(bell);
    }
}

int bfm_tower_interior_bell_actor(BfmTowerInteriorBell *bell, unsigned elapsed_updates,
                                  BfmForestActor *out)
{
    if (!bell || !out) return 0;
    /* Constant angular acceleration opposes the displacement. Zero belongs
     * to the rising half, making the two arcs two updates different in length. */
    unsigned pitch = 0;
    if (elapsed_updates > 20) {
        unsigned half = 2 * (bell->speed / bell->acceleration + 1);
        unsigned t = (elapsed_updates - 20) % (2 * half);
        int64_t angle;
        if (t <= half + 1) {
            angle = (int64_t)t * (bell->speed + bell->acceleration) -
                    (int64_t)bell->acceleration * t * ((int)t - 1) / 2;
        } else {
            t -= half + 1;
            angle = -(int64_t)t * bell->speed +
                    (int64_t)bell->acceleration * t * (t - 1) / 2;
        }
        pitch = (unsigned)angle & 4095;
    }
    bfm_forest_actor_pitch_yaw(pitch, bell->yaw, bell->position, &bell->root);
    *out = (BfmForestActor){.mesh=bell->mesh, .pose=&bell->pose,
                            .root=&bell->root, .visible_parts=1};
    return 1;
}

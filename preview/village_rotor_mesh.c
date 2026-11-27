/* SPDX-License-Identifier: MIT */
#include "village_rotor_mesh.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

struct BfmVillageRotorMesh {
    MusashiForestPlayerMesh *model;
    int16_t position[3];
    unsigned heading;
};

BfmVillageRotorMesh *bfm_village_rotor_mesh_load(const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t room = UINT32_C(0x8018c794) - base;
    const size_t station = UINT32_C(0x80189cfc) - base;
    if (!scene || bytes < room + 0xbc || bytes < station + 8) return NULL;
    uint32_t list = u32le(scene + room + 0xb4);
    uint32_t end = u32le(scene + room + 0xb8);
    if (list < base || end <= list || end - base > bytes || (end - list) % 20) return NULL;
    unsigned found = 0;
    for (size_t i = list - base; i < end - base; i += 20) {
        unsigned kind = u16le(scene + i + 6);
        if (!kind) break;
        if (kind == 500) {
            if (found || u16le(scene + i + 8)) return NULL;
            found = 1;
        }
    }
    if (!found) return NULL;

    BfmVillageRotorMesh *mesh = calloc(1, sizeof *mesh);
    if (!mesh) return NULL;
    unsigned count;
    mesh->model = musashi_forest_player_mesh_from_table(scene, bytes, base,
                                                        UINT32_C(0x801a58a4), &count);
    if (!mesh->model || count != 1) {
        bfm_village_rotor_mesh_free(mesh);
        return NULL;
    }
    for (unsigned axis = 0; axis < 3; ++axis)
        mesh->position[axis] = s16le(scene + station + 2 * axis);
    mesh->heading = u16le(scene + station + 6);
    return mesh;
}

void bfm_village_rotor_mesh_free(BfmVillageRotorMesh *mesh)
{
    if (mesh) {
        musashi_forest_player_mesh_free(mesh->model);
        free(mesh);
    }
}

int bfm_village_rotor_mesh_append(const BfmVillageRotorMesh *mesh, BfmBedroomFigure *figure,
                                 const BfmBedroomView *view, unsigned yaw)
{
    if (!mesh) return 0;
    /* The vanes and hub share a single rigid part and turn together in X/Z. */
    static const BfmForestPose rigid = {
        .local = {{.rotation = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}}}}
    };
    BfmForestTransform root;
    bfm_forest_actor_root(mesh->heading + yaw, mesh->position, &root);
    BfmForestActor rotor = {
        .mesh = mesh->model, .root = &root, .pose = &rigid, .visible_parts = 1
    };
    return bfm_bedroom_figure_add_actor(figure, view, &rotor, NULL, NULL, 0);
}

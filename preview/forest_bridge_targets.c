/* SPDX-License-Identifier: MIT */
#include "forest_bridge_targets.h"
#include "forest_bridge_floor.h"
#include "forest_opening_data.h"
#include "forest_trail.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int16_t base[3];
    unsigned yaw;
    BfmForestTransform root;
    BfmForestActor draw;
} BridgeTrunk;

struct BfmForestBridgeTargets {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose rigid;
    BfmForestGuardBox body;
    BridgeTrunk trunks[BFM_FOREST_BRIDGE_TARGET_COUNT];
};

BfmForestBridgeTargets *bfm_forest_bridge_targets_open(const BfmForestField *field)
{
    const uint8_t *rows = bfm_forest_room_bytes(field, UINT32_C(0x801b1c94),
                                                BFM_FOREST_BRIDGE_TARGET_COUNT * 20);
    BfmForestBridgeFloor shape;
    if (!rows || !bfm_forest_bridge_floor_load(field, &shape)) return NULL;
    BfmForestBridgeTargets *targets = calloc(1, sizeof(*targets));
    if (!targets) return NULL;
    targets->mesh = bfm_forest_trunk_mesh(field, UINT32_C(0x801d1cbc));
    if (!targets->mesh) goto failed;

    targets->body = shape.box;
    for (unsigned axis = 0; axis < 3; ++axis)
        targets->rigid.local[0].rotation[axis][axis] = 4096;
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        const uint8_t *row = rows + 20 * role;
        if (u16le(row + 6) != 16 || row[8] != 4 || row[9] != role) goto failed;
        BridgeTrunk *trunk = &targets->trunks[role];
        for (unsigned axis = 0; axis < 3; ++axis) trunk->base[axis] = s16le(row + 2 * axis);
        trunk->yaw = u16le(row + 12);
        bfm_forest_actor_root(trunk->yaw, trunk->base, &trunk->root);
        trunk->draw = (BfmForestActor){targets->mesh, &targets->rigid, &trunk->root, 1, NULL};
    }
    return targets;
failed:
    bfm_forest_bridge_targets_free(targets);
    return NULL;
}

void bfm_forest_bridge_targets_free(BfmForestBridgeTargets *targets)
{
    if (!targets) return;
    musashi_forest_player_mesh_free(targets->mesh);
    free(targets);
}

const BfmForestActor *bfm_forest_bridge_targets_actor(const BfmForestBridgeTargets *targets,
                                                      unsigned role)
{
    return targets && role < BFM_FOREST_BRIDGE_TARGET_COUNT ? &targets->trunks[role].draw : NULL;
}

void bfm_forest_bridge_targets_pitch(BfmForestBridgeTargets *targets, unsigned role,
                                     unsigned pitch)
{
    if (!targets || role >= BFM_FOREST_BRIDGE_TARGET_COUNT) return;
    BridgeTrunk *trunk = &targets->trunks[role];
    bfm_forest_actor_pitch_yaw(pitch, trunk->yaw, trunk->base, &trunk->root);
}

unsigned bfm_forest_bridge_targets_impact(const BfmForestBridgeTargets *targets,
                                           unsigned eligible, const int16_t from[3],
                                           const int16_t to[3], int16_t hit[3], int16_t normal[3])
{
    if (!targets || !from || !to || !hit) return 0;
    int16_t end[3];
    memcpy(end, to, sizeof(end));
    unsigned touched = 0;
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        unsigned bit = 1u << role;
        int16_t center[3];
        if ((eligible & bit) &&
            bfm_forest_guard_contact_point(&targets->body, &targets->trunks[role].root,
                                           from, end, center, end, normal))
            touched |= bit;
    }
    if (touched) memcpy(hit, end, sizeof(end));
    return touched;
}

unsigned bfm_forest_bridge_targets_contact(const BfmForestBridgeTargets *targets,
                                           unsigned eligible, const int16_t from[3],
                                           const int16_t to[3], int16_t hit[3])
{
    return bfm_forest_bridge_targets_impact(targets, eligible, from, to, hit, NULL);
}

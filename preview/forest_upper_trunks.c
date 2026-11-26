/* SPDX-License-Identifier: MIT */
#include "forest_upper_trunks.h"
#include "forest_bridge.h"
#include "forest_opening_data.h"
#include "forest_trail.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>

typedef struct {
    BfmForestLog log;
    BfmForestTransform root;
    BfmForestActor draw;
    int16_t pivot[3];
    unsigned yaw, delay;
} UpperTrunk;

struct BfmForestUpperTrunks {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose rigid;
    BfmForestBridgeFloor floor;
    int fall_distance;
    UpperTrunk row[BFM_FOREST_UPPER_TRUNKS];
};

BfmForestUpperTrunks *bfm_forest_upper_trunks_open(const BfmForestField *field)
{
    const uint8_t *rows = bfm_forest_room_bytes(field, UINT32_C(0x801b1c58),
                                               BFM_FOREST_UPPER_TRUNKS * 20);
    const uint8_t *range = bfm_forest_room_bytes(field, UINT32_C(0x8018a8f8), 4);
    if (!rows || !range || !u32le(range) || u32le(range) > INT16_MAX) return NULL;

    BfmForestUpperTrunks *trunks = calloc(1, sizeof(*trunks));
    if (!trunks) return NULL;
    trunks->fall_distance = (int)u32le(range);
    if (!bfm_forest_bridge_floor_load_at(field, UINT32_C(0x8018a8a4), &trunks->floor))
        goto fail;
    trunks->mesh = bfm_forest_trunk_mesh(field, UINT32_C(0x801d15c4));
    if (!trunks->mesh) goto fail;
    for (unsigned axis = 0; axis < 3; ++axis)
        trunks->rigid.local[0].rotation[axis][axis] = 4096;
    for (unsigned i = 0; i < BFM_FOREST_UPPER_TRUNKS; ++i) {
        const uint8_t *place = rows + 20 * i;
        if (u16le(place + 6) != 16 || u16le(place + 8) != i + 1) goto fail;
        UpperTrunk *trunk = &trunks->row[i];
        for (unsigned axis = 0; axis < 3; ++axis)
            trunk->pivot[axis] = s16le(place + 2 * axis);
        trunk->yaw = u16le(place + 12);
        bfm_forest_actor_root(trunk->yaw, trunk->pivot, &trunk->root);
        trunk->draw = (BfmForestActor){trunks->mesh, &trunks->rigid, &trunk->root, 1, NULL};
    }
    return trunks;
fail:
    bfm_forest_upper_trunks_free(trunks);
    return NULL;
}

void bfm_forest_upper_trunks_free(BfmForestUpperTrunks *trunks)
{
    if (trunks) musashi_forest_player_mesh_free(trunks->mesh);
    free(trunks);
}

void bfm_forest_upper_trunks_tick(BfmForestUpperTrunks *trunks, int player_z)
{
    for (unsigned i = 0; i < BFM_FOREST_UPPER_TRUNKS; ++i) {
        UpperTrunk *trunk = &trunks->row[i];
        BfmForestLog *log = &trunk->log;
        int distance = abs(player_z - trunk->pivot[2]);
        if (log->state == BFM_LOG_UNPLACED && distance < 768) {
            log->state = BFM_LOG_STANDING;
            log->flags = 0xc800;
        } else if (log->state == BFM_LOG_STANDING && distance < trunks->fall_distance) {
            log->state = BFM_LOG_FALLING;
            log->flags &= ~0x8c00u;
            log->velocity = 16 << 16;
            trunk->delay = 16;
        } else if (log->state == BFM_LOG_FALLING && (!trunk->delay || !--trunk->delay)) {
            bfm_forest_log_fall(log);
            bfm_forest_actor_pitch_yaw((unsigned)log->pitch, trunk->yaw,
                                       trunk->pivot, &trunk->root);
        }
    }
}

const BfmForestActor *bfm_forest_upper_trunk(const BfmForestUpperTrunks *trunks,
                                            unsigned index)
{
    if (index >= BFM_FOREST_UPPER_TRUNKS || trunks->row[index].log.state == BFM_LOG_UNPLACED)
        return NULL;
    return &trunks->row[index].draw;
}

int bfm_forest_upper_trunks_surface(const BfmForestUpperTrunks *trunks,
    BfmForestBridgeProbe probe, const int16_t from[3], const int16_t to[3],
    BfmForestBridgeSurface *out)
{
    int found = 0;
    for (unsigned i = 0; i < BFM_FOREST_UPPER_TRUNKS; ++i) {
        const UpperTrunk *trunk = &trunks->row[i];
        BfmForestBridgeSurface surface;
        if (bfm_forest_bridge_floor_contact(&trunks->floor, &trunk->root, trunk->log.state,
                trunk->log.flags, probe, from, to, &surface) && (!found || surface.height < out->height)) {
            *out = surface;
            found = 1;
        }
    }
    return found;
}

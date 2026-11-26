/* SPDX-License-Identifier: MIT */
#include "forest_shore.h"
#include "forest_shore_motion.h"
#include "forest_guard_clip.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

struct BfmForestShore {
    int16_t spawn[3];
    int active, beyond_bank;
    MusashiForestPlayerMesh *mesh;
    BfmForestGuardClip walk, notice;
    BfmForestShoreMotion motion;
    BfmForestMarker marker;
    int marker_key_seen, previously_close;
    BfmForestTransform root;
    BfmForestPose pose;
    BfmForestActor draw;
};

BfmForestShore *bfm_forest_shore_open(const BfmForestField *field,
    const uint8_t *scene, size_t bytes, uint32_t origin, const uint8_t *descriptor)
{
    /* First creek soldier in the room's distance-activated placements. */
    if (!field || !scene || !descriptor || u32le(descriptor + 0xbc) != UINT32_C(0x8013e298))
        return NULL;
    uint32_t list = u32le(descriptor + 0xb8);
    if (list > UINT32_MAX - 40) return NULL;
    const uint8_t *placement = bfm_forest_room_bytes(field, list + 2 * 20, 20);
    if (!placement || u16le(placement + 6) != 47 || u16le(placement + 8) ||
        u16le(placement + 12)) return NULL;
    BfmForestShore *shore = calloc(1, sizeof *shore);
    if (!shore) return NULL;
    for (unsigned i = 0; i < 3; ++i) shore->spawn[i] = s16le(placement + 2 * i);
    unsigned parts;
    shore->mesh = musashi_forest_player_mesh_from_table(scene, bytes, origin,
        UINT32_C(0x801a0058), &parts);
    if (!shore->mesh || parts != BFM_GUARD_PARTS ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801aff18), 0xffff, &shore->walk) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b0048), 0xffff, &shore->notice)) {
        bfm_forest_shore_free(shore);
        return NULL;
    }
    shore->draw = (BfmForestActor){shore->mesh, &shore->pose, &shore->root, (1u << BFM_GUARD_PARTS) - 1};
    return shore;
}

void bfm_forest_shore_free(BfmForestShore *shore)
{
    if (shore) {
        musashi_forest_player_mesh_free(shore->mesh);
        free(shore);
    }
}

/* In the ordinary frontal encounter the first notice key 11 coincides
 * with the grounded player's approach into this measured separation.
 * Issue only on that observed combination, once for this placed guard. */
static int step_marker(BfmForestShore *shore, const BfmForestField *field,
                       const int16_t player[3])
{
    const BfmForestShoreMotion *m = &shore->motion;
    bfm_forest_marker_advance(&shore->marker, m->position);
    if (m->phase != BFM_SHORE_NOTICE || shore->marker_key_seen) return 1;
    int64_t dx = (int)m->position[0] - player[0], dz = (int)m->position[2] - player[2];
    int close = m->position[1] == player[1] && dz > 0 &&
                dx * dx + dz * dz <= 38 * 38 + 134 * 134;
    if (m->frame == 11) {
        shore->marker_key_seen = 1;
        if (close && !shore->previously_close)
            return bfm_forest_marker_start(&shore->marker, field, m->position);
    }
    shore->previously_close = close;
    return 1;
}

BfmForestShoreUpdate bfm_forest_shore_step(BfmForestShore *shore, const BfmForestField *field,
                          const int16_t player[3])
{
    if (!shore || !field || !player) return BFM_SHORE_INVALID;
    if (shore->beyond_bank) return BFM_SHORE_UNRESOLVED;
    if (!shore->active) {
        int gap = abs((int)player[2] - shore->spawn[2]);
        /* The room-selected placement predicate admits this Z band. */
        if (gap < 705 || gap >= 768) return BFM_SHORE_UPDATED;
        if (!bfm_forest_shore_begin(&shore->motion, field, shore->spawn)) return BFM_SHORE_INVALID;
    } else {
        unsigned count = shore->motion.phase == BFM_SHORE_WALK ? shore->walk.count : shore->notice.count;
        if (!bfm_forest_shore_tick(&shore->motion, field, player, count)) {
            /* The known walk covers the far slope and bank. Other footing
             * needs its own actor action and must not interrupt the player. */
            shore->beyond_bank = 1;
            return BFM_SHORE_UNRESOLVED;
        }
    }
    const BfmForestShoreMotion *motion = &shore->motion;
    const BfmForestGuardClip *clip = motion->phase == BFM_SHORE_WALK ? &shore->walk : &shore->notice;
    if (motion->frame >= clip->count ||
        !bfm_forest_actor_pose_decode(clip->pose[motion->frame], BFM_GUARD_POSE_BYTES, BFM_GUARD_PARTS, &shore->pose))
        return BFM_SHORE_INVALID;
    bfm_forest_actor_root(motion->yaw, motion->position, &shore->root);
    shore->active = 1;
    return step_marker(shore, field, player) ? BFM_SHORE_UPDATED : BFM_SHORE_INVALID;
}

const BfmForestActor *bfm_forest_shore_actor(const BfmForestShore *shore)
{
    return shore && shore->active && !shore->beyond_bank ? &shore->draw : NULL;
}

const BfmForestMarker *bfm_forest_shore_marker(const BfmForestShore *shore)
{
    return bfm_forest_shore_actor(shore) && shore->marker.reaction.scale > 0 ? &shore->marker : NULL;
}

const MusashiForestPlayerMesh *bfm_forest_shore_guard_mesh(const BfmForestShore *shore)
{
    return shore ? shore->mesh : NULL;
}

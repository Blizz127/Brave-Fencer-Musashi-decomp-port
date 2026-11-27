/* SPDX-License-Identifier: MIT */
#include "forest_shore.h"
#include "forest_shore_motion.h"
#include "forest_guard_clip.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

struct BfmForestShore {
    int16_t spawn[3], spawn_yaw;
    int owns_mesh;
    uint16_t health, spawn_health, last_attack;
    int active, beyond_bank;
    MusashiForestPlayerMesh *mesh;
    BfmForestGuardClip walk, notice, recoil, pursuit, strike, pause, follow_through;
    BfmForestShoreMotion motion;
    int recoil_pending, striking;
    BfmForestMarker marker;
    int marker_key_seen;
    BfmForestTransform root;
    BfmForestPose pose;
    BfmForestActor draw;
};

static BfmForestShore *new_placement(const uint8_t placement[20], uint16_t health)
{
    if (!placement || u16le(placement + 6) != 47 || u16le(placement + 8) != 0) return NULL;
    BfmForestShore *shore = calloc(1, sizeof *shore);
    if (shore) {
        for (unsigned axis = 0; axis < 3; ++axis) shore->spawn[axis] = s16le(placement + 2 * axis);
        shore->spawn_yaw = s16le(placement + 12);
        shore->health = shore->spawn_health = health;
    }
    return shore;
}

static void set_guard_model(BfmForestShore *shore)
{
    shore->draw = (BfmForestActor){shore->mesh, &shore->pose, &shore->root,
                                   (1u << BFM_GUARD_PARTS) - 1};
}

BfmForestShore *bfm_forest_shore_open_patrol(const BfmForestShore *primary,
                                           const uint8_t placement[20])
{
    if (!primary) return NULL;
    BfmForestShore *shore = new_placement(placement, primary->spawn_health);
    if (shore) {
        shore->mesh = primary->mesh;
        shore->walk = primary->walk;
        shore->notice = primary->notice;
        shore->pursuit = primary->pursuit;
        shore->strike = primary->strike;
        shore->pause = primary->pause;
        shore->follow_through = primary->follow_through;
        set_guard_model(shore);
    }
    return shore;
}

BfmForestShore *bfm_forest_shore_open(const BfmForestField *field,
    const uint8_t *scene, size_t bytes, uint32_t origin, const uint8_t *descriptor)
{
    /* First creek soldier in the room's distance-activated placements. */
    if (!field || !scene || !descriptor || u32le(descriptor + 0xbc) != UINT32_C(0x8013e298) ||
        u32le(descriptor + 0xc0) != UINT32_C(0x8012bf7c))
        return NULL;
    uint32_t list = u32le(descriptor + 0xb8);
    if (list > UINT32_MAX - 40) return NULL;
    const uint8_t *placement = bfm_forest_room_bytes(field, list + 2 * 20, 20);
    /* The admitted placement has variant zero. Its soldier descriptor
     * starts with the health assigned when this guard appears. */
    const uint8_t *stats = bfm_forest_room_bytes(field, UINT32_C(0x8018abc0), 2);
    if (!stats || s16le(stats) <= 0) return NULL;
    BfmForestShore *shore = new_placement(placement, u16le(stats));
    if (!shore) return NULL;
    shore->owns_mesh = 1;
    unsigned parts;
    shore->mesh = musashi_forest_player_mesh_from_table(scene, bytes, origin,
        UINT32_C(0x801a0058), &parts);
    if (!shore->mesh || parts != BFM_GUARD_PARTS ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801aff18), 0xffff, &shore->walk) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b0048), 0xffff, &shore->notice) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801affc0), 0xffff, &shore->pursuit) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b0118), 0xffff, &shore->strike) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801afd28), 0xffff, &shore->pause) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b01e0), 0xffff, &shore->follow_through)) {
        bfm_forest_shore_free(shore);
        return NULL;
    }
    set_guard_model(shore);
    return shore;
}

static void start_recoil(BfmForestShore *shore, uint16_t attacker_yaw)
{
    shore->motion.yaw = (uint16_t)((attacker_yaw + 2048) & 4095);
    shore->motion.phase = BFM_SHORE_RECOIL;
    shore->motion.frame = 0;
    shore->motion.recoil_vy_q16 = 0;
    shore->recoil_pending = 1;
}

void bfm_forest_shore_recoil(BfmForestShore *shore, uint16_t attacker_yaw)
{
    if (bfm_forest_shore_actor(shore) && shore->motion.phase != BFM_SHORE_RECOIL)
        start_recoil(shore, attacker_yaw);
}

int bfm_forest_shore_take_typed_hit(BfmForestShore *shore, uint16_t attacker_yaw,
                                    uint16_t strength, uint16_t attack_type)
{
    if (!bfm_forest_shore_actor(shore) || !attack_type || shore->last_attack == attack_type ||
        !strength || strength >= shore->health) return 0;
    shore->last_attack = attack_type;
    shore->health -= strength;
    start_recoil(shore, attacker_yaw);
    return 1;
}

int bfm_forest_shore_take_hit(BfmForestShore *shore, uint16_t attacker_yaw, uint16_t strength)
{
    if (!bfm_forest_shore_actor(shore) || shore->motion.phase == BFM_SHORE_RECOIL || strength >= shore->health)
        return 0;
    shore->last_attack = 1;
    shore->health -= strength;
    start_recoil(shore, attacker_yaw);
    return 1;
}

static const BfmForestGuardClip *current_clip(const BfmForestShore *shore)
{
    switch (shore->motion.phase) {
    case BFM_SHORE_NOTICE: return &shore->notice;
    case BFM_SHORE_PURSUIT: return &shore->pursuit;
    case BFM_SHORE_STRIKE: return &shore->strike;
    case BFM_SHORE_PAUSE: return &shore->pause;
    case BFM_SHORE_FOLLOW_THROUGH: return &shore->follow_through;
    case BFM_SHORE_RECOIL: return &shore->recoil;
    default: return &shore->walk;
    }
}

void bfm_forest_shore_free(BfmForestShore *shore)
{
    if (shore) {
        if (shore->owns_mesh) musashi_forest_player_mesh_free(shore->mesh);
        free(shore);
    }
}

/* On entering the first notice action, the guard starts a 25-update timer.
 * Its mounted animation reaches key 11 when the action observes 15 ticks left. */
static int step_marker(BfmForestShore *shore, const BfmForestField *field)
{
    const BfmForestShoreMotion *m = &shore->motion;
    bfm_forest_marker_advance(&shore->marker, m->position);
    if (m->phase != BFM_SHORE_NOTICE || m->frame != 11 || shore->marker_key_seen)
        return 1;
    shore->marker_key_seen = 1;
    return bfm_forest_marker_start(&shore->marker, field, m->position, -120);
}

BfmForestShoreUpdate bfm_forest_shore_step(BfmForestShore *shore, const BfmForestField *field,
                          const int16_t player[3], const int16_t target[3], BfmGameplayRun *run)
{
    if (!shore || !field || !player || !target) return BFM_SHORE_INVALID;
    shore->striking = 0;
    if (shore->active && abs((int)player[2] - shore->motion.position[2]) >= 864) {
        shore->active = 0;
        shore->beyond_bank = 0;
        shore->recoil_pending = 0;
        shore->marker_key_seen = 0;
        shore->marker = (BfmForestMarker){0};
        return BFM_SHORE_UPDATED;
    }
    if (shore->beyond_bank) return BFM_SHORE_UNRESOLVED;
    int first_hit = shore->recoil_pending;
    BfmForestShorePhase prior_phase = shore->motion.phase;
    if (first_hit &&
        (!bfm_forest_guard_clip_read(field, UINT32_C(0x801b02a8), 0xffff, &shore->recoil) ||
         shore->recoil.count != 16))
        return BFM_SHORE_INVALID;
    int16_t prior_y = shore->motion.position[1];
    if (!shore->active) {
        int gap = abs((int)player[2] - shore->spawn[2]);
        /* The room-selected placement predicate admits this Z band. */
        if (gap < 705 || gap >= 768) return BFM_SHORE_UPDATED;
        if (!bfm_forest_shore_begin(&shore->motion, field, shore->spawn)) return BFM_SHORE_INVALID;
        shore->motion.yaw = (uint16_t)shore->spawn_yaw;
        shore->health = shore->spawn_health;
        shore->last_attack = 0;
    } else {
        shore->striking = !first_hit &&
            ((prior_phase == BFM_SHORE_STRIKE && shore->motion.patrol_timer == 7) ||
             (prior_phase == BFM_SHORE_FOLLOW_THROUGH && shore->motion.patrol_timer == 10));
        int updated = first_hit ? bfm_forest_shore_recoil_height(&shore->motion, field) :
            bfm_forest_shore_tick(&shore->motion, field, target, current_clip(shore)->count, run);
        shore->recoil_pending = 0;
        if (!updated) {
            /* The known walk covers the far slope and bank. Other footing
             * needs its own actor action and must not interrupt the player. */
            shore->beyond_bank = 1;
            return BFM_SHORE_UNRESOLVED;
        }
    }
    const BfmForestShoreMotion *motion = &shore->motion;
    if (prior_phase == BFM_SHORE_RECOIL && motion->phase != BFM_SHORE_RECOIL) shore->last_attack = 0;
    const BfmForestGuardClip *clip = current_clip(shore);
    if (motion->frame >= clip->count ||
        !bfm_forest_actor_pose_decode(clip->pose[motion->frame], BFM_GUARD_POSE_BYTES, BFM_GUARD_PARTS, &shore->pose))
        return BFM_SHORE_INVALID;
    bfm_forest_actor_root(motion->yaw, motion->position, &shore->root);
    /* The first hit pose draws at the preceding standing height. */
    if (first_hit) shore->root.translation[1] = prior_y;
    if (run && prior_phase == BFM_SHORE_WALK && motion->phase == BFM_SHORE_TURN) {
        /* The guard restarts patrol, then replaces that random interval
         * with the fixed turn duration and draws a fresh notice delay. */
        bfm_gameplay_random(run);
        shore->motion.notice_delay = 30 + (bfm_gameplay_random(run) & 31);
    }
    int resumed_walk = motion->phase == BFM_SHORE_WALK &&
        prior_phase != BFM_SHORE_WALK;
    if (resumed_walk) {
        shore->marker_key_seen = 0;
        shore->motion.walk_updates = 0;
        shore->motion.turn_target = motion->yaw;
        shore->motion.travel_yaw = motion->yaw;
    }
    if (run && (!shore->active || resumed_walk))
        shore->motion.patrol_timer = 70 + (bfm_gameplay_random(run) & 31);
    shore->active = 1;
    return step_marker(shore, field) ? BFM_SHORE_UPDATED : BFM_SHORE_INVALID;
}

const BfmForestActor *bfm_forest_shore_actor(const BfmForestShore *shore)
{
    return shore && shore->active && !shore->beyond_bank ? &shore->draw : NULL;
}

const BfmForestTransform *bfm_forest_shore_strike(const BfmForestShore *shore, int16_t *yaw)
{
    if (!yaw || !bfm_forest_shore_actor(shore) || !shore->striking) return NULL;
    *yaw = (int16_t)shore->motion.yaw;
    return &shore->root;
}

const BfmForestMarker *bfm_forest_shore_marker(const BfmForestShore *shore)
{
    return bfm_forest_shore_actor(shore) && shore->marker.reaction.scale > 0 ? &shore->marker : NULL;
}

const MusashiForestPlayerMesh *bfm_forest_shore_guard_mesh(const BfmForestShore *shore)
{
    return shore ? shore->mesh : NULL;
}

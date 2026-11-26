/* SPDX-License-Identifier: MIT */
#include "forest_trail.h"
#include "forest_guard_clip.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>

enum TrunkState { UNPLACED, STANDING, FALLING, SETTLED };
enum GuardState { WAITING, APPROACHING, SWINGING, TURNING, RETREATING, BEYOND_OBSERVED_RUN };

struct BfmForestTrail {
    enum TrunkState trunk;
    enum GuardState guard;
    int16_t base[3], guard_at[3];
    int16_t spawn_floor;
    int trigger_distance, yaw, guard_yaw, pitch, angular_speed;
    unsigned delay, bounces, key, scatter;
    BfmForestGuardClip run, swing;
    MusashiForestPlayerMesh *wood;
    BfmForestTransform trunk_root, guard_root;
    BfmForestPose rigid, guard_pose;
    BfmForestActor trunk_draw, guard_draw;
};

static MusashiForestPlayerMesh *load_trunk(const BfmForestField *field)
{
    const uint32_t table = UINT32_C(0x801d15c4);
    const uint8_t *list = bfm_forest_room_bytes(field, table, 8);
    if (!list || u32le(list + 4) != 0) return NULL;
    uint32_t first = u32le(list);
    if (first >= table || table - first > (1u << 20)) return NULL;
    size_t bytes = table - first + 8;
    const uint8_t *data = bfm_forest_room_bytes(field, first, bytes);
    unsigned parts = 0;
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_from_table(data, bytes, first, table, &parts);
    if (parts == 1) return mesh;
    musashi_forest_player_mesh_free(mesh);
    return NULL;
}

BfmForestTrail *bfm_forest_trail_open(const BfmForestField *field, const BfmForestShore *shore)
{
    const uint8_t *row = bfm_forest_room_bytes(field, UINT32_C(0x801b1c44), 20);
    const uint8_t *range = bfm_forest_room_bytes(field, UINT32_C(0x8018a8f8), 4);
    const MusashiForestPlayerMesh *guard = bfm_forest_shore_guard_mesh(shore);
    if (!row || !range || !guard || u16le(row + 6) != 16 || u16le(row + 8) ||
        u16le(row + 12) != 1024 || !u32le(range) || u32le(range) > INT16_MAX) return NULL;
    BfmForestTrail *trail = calloc(1, sizeof(*trail));
    if (!trail) return NULL;
    for (unsigned i = 0; i < 3; ++i) trail->base[i] = s16le(row + 2 * i);
    if (trail->base[0] > INT16_MAX - 768 || trail->base[2] > INT16_MAX - 32) goto failed;
    BfmForestContact floor;
    if (!bfm_forest_ground(field, trail->base[0] + 240, trail->base[1],
                           trail->base[2] + 32, &floor)) goto failed;
    trail->spawn_floor = floor.height;
    trail->yaw = u16le(row + 12);
    trail->trigger_distance = (int)u32le(range);
    trail->wood = load_trunk(field);
    if (!trail->wood ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801affc0), 0xffff, &trail->run) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x8018ab24), 0xfffe, &trail->swing) ||
        trail->run.count != 16 || trail->swing.count != 12) goto failed;
    for (unsigned i = 0; i < 3; ++i) trail->rigid.local[0].rotation[i][i] = 4096;
    bfm_forest_actor_root(trail->yaw, trail->base, &trail->trunk_root);
    trail->trunk_draw = (BfmForestActor){trail->wood, &trail->rigid, &trail->trunk_root, 1};
    trail->guard_draw = (BfmForestActor){guard, &trail->guard_pose, &trail->guard_root,
                                        (1u << BFM_GUARD_PARTS) - 1};
    return trail;
failed:
    bfm_forest_trail_free(trail);
    return NULL;
}

void bfm_forest_trail_free(BfmForestTrail *trail)
{
    if (trail) musashi_forest_player_mesh_free(trail->wood);
    free(trail);
}

static void advance_trunk(BfmForestTrail *trail)
{
    if (trail->scatter) --trail->scatter;
    if (trail->trunk != FALLING) return;
    if (trail->delay && --trail->delay) return;
    if (trail->pitch >= 1024) {
        trail->pitch = 1024;
        trail->angular_speed = -trail->angular_speed / 2;
        ++trail->bounces;
    }
    trail->angular_speed += 4;
    trail->pitch += trail->angular_speed;
    if (trail->bounces == 2 && trail->pitch >= 1024) {
        trail->trunk = SETTLED;
        trail->scatter = 16;
        trail->pitch = 1024;
    }
    bfm_forest_actor_pitch_yaw(trail->pitch, trail->yaw, trail->base, &trail->trunk_root);
}

static int update_guard_pose(BfmForestTrail *trail)
{
    const BfmForestGuardClip *clip = trail->guard == SWINGING ? &trail->swing : &trail->run;
    if (!bfm_forest_actor_pose_decode(clip->pose[trail->key], BFM_GUARD_POSE_BYTES,
                                     BFM_GUARD_PARTS, &trail->guard_pose)) return 0;
    bfm_forest_actor_root((unsigned)trail->guard_yaw, trail->guard_at, &trail->guard_root);
    return 1;
}

static int advance_guard(BfmForestTrail *trail)
{
    switch (trail->guard) {
    case WAITING:
    case BEYOND_OBSERVED_RUN:
        return 1;
    case APPROACHING:
        trail->guard_at[0] -= 12;
        if (++trail->key == trail->run.count - 1) {
            trail->guard = SWINGING;
            trail->key = 0;
        }
        break;
    case SWINGING:
        if (++trail->key == trail->swing.count) {
            trail->guard = TURNING;
            trail->key = 0;
        }
        break;
    case TURNING:
        ++trail->key;
        trail->guard_yaw -= 512;
        if (trail->guard_yaw == -1024) trail->guard = RETREATING;
        break;
    case RETREATING:
        /* The positive run ends 768 units to the right of the parent. The
         * original slot was reused at the next update on this approach. */
        if (trail->guard_at[0] >= trail->base[0] + 768) {
            trail->guard = BEYOND_OBSERVED_RUN;
            return 1;
        }
        trail->guard_at[0] += 12;
        trail->key = (trail->key + 1) % trail->run.count;
        break;
    }
    return update_guard_pose(trail);
}

BfmForestTrailUpdate bfm_forest_trail_step(BfmForestTrail *trail, const int16_t player[3])
{
    if (!trail || !player) return 0;
    int distance = abs((int)player[2] - trail->base[2]);
    if (trail->trunk == UNPLACED && distance < 768) trail->trunk = STANDING;
    if (trail->trunk == STANDING && distance < trail->trigger_distance) {
        trail->trunk = FALLING;
        trail->delay = 16;
        trail->angular_speed = 16;
        trail->guard = APPROACHING;
        trail->guard_yaw = trail->yaw;
        /* Use the floor under the issued child placement, before the first
         * 12-unit step along -Z. The lateral run keeps its initial height. */
        trail->guard_at[0] = trail->base[0] + 240;
        trail->guard_at[1] = trail->spawn_floor;
        trail->guard_at[2] = trail->base[2] + 20;
        return update_guard_pose(trail) ? BFM_TRAIL_UPDATED : BFM_TRAIL_INVALID;
    }
    advance_trunk(trail);
    if (!advance_guard(trail)) return BFM_TRAIL_INVALID;
    return trail->guard == BEYOND_OBSERVED_RUN ? BFM_TRAIL_GUARD_BEYOND_RUN : BFM_TRAIL_UPDATED;
}

unsigned bfm_forest_trail_scatter(const BfmForestTrail *trail)
{
    return trail ? trail->scatter : 0;
}

const BfmForestActor *bfm_forest_trail_trunk(const BfmForestTrail *trail)
{
    return trail && trail->trunk != UNPLACED ? &trail->trunk_draw : NULL;
}

const BfmForestActor *bfm_forest_trail_guard(const BfmForestTrail *trail)
{
    return trail && trail->guard != WAITING && trail->guard != BEYOND_OBSERVED_RUN ? &trail->guard_draw : NULL;
}

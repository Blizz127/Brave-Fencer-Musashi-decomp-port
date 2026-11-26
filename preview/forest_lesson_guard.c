/* SPDX-License-Identifier: MIT */
#include "forest_lesson_guard.h"
#include "forest_guard_clip.h"
#include "forest_pose.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

struct BfmForestLessonGuard {
    MusashiForestPlayerMesh *mesh;
    BfmForestTransform root;
    BfmForestPose pose;
    BfmForestGuardClip fire, lower, idle;
    const BfmForestGuardClip *current;
    unsigned repeats;
    int tethered;
    BfmForestGuardBox box;
    BfmForestActor draw;
    unsigned key;
};

BfmForestLessonGuard *bfm_forest_lesson_guard_spawn(const BfmForestField *field)
{
    const uint8_t *spawn = bfm_forest_room_bytes(field, UINT32_C(0x8018a868), 20);
    if (!spawn || u16le(spawn + 6) != 47 || u16le(spawn + 8) != 0x0500) return NULL;
    unsigned variant = spawn[9];
    const uint8_t *profile = bfm_forest_room_bytes(field,
        UINT32_C(0x8018abc0) + variant * 52, 52);
    if (!profile || s16le(profile + 44) != 8) return NULL;
    const uint8_t *shape = bfm_forest_room_bytes(field, u32le(profile + 8), 16);
    uint32_t table = u32le(profile + 4);
    const unsigned table_size = 4 * (BFM_GUARD_PARTS + 1);
    const uint8_t *parts = bfm_forest_room_bytes(field, table, table_size);
    if (!shape || u32le(shape) || !parts || u32le(parts + table_size - 4)) return NULL;
    uint32_t first = u32le(parts);
    if (first >= table || table - first > (1u << 20)) return NULL;
    size_t bytes = table - first + table_size;
    const uint8_t *body = bfm_forest_room_bytes(field, first, bytes);
    if (!body) return NULL;
    BfmForestLessonGuard *guard = calloc(1, sizeof(*guard));
    if (!guard) return NULL;
    unsigned count;
    guard->mesh = musashi_forest_player_mesh_from_table(body, bytes, first, table, &count);
    if (!guard->mesh || count != BFM_GUARD_PARTS ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b0d50), 0xffff, &guard->fire) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b0e48), 0xffff, &guard->lower) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801afdd0), 0xffff, &guard->idle))
        goto failed;
    int16_t at[3];
    for (unsigned i = 0; i < 3; ++i) {
        at[i] = s16le(spawn + 2*i);
        guard->box.low[i] = s16le(shape + 4 + 4*i);
        guard->box.high[i] = s16le(shape + 6 + 4*i);
        if (guard->box.low[i] > guard->box.high[i]) goto failed;
    }
    BfmForestContact floor;
    if (!bfm_forest_ground(field, at[0], at[1], at[2], &floor)) goto failed;
    at[1] = floor.height;
    bfm_forest_actor_root(u16le(spawn + 12), at, &guard->root);
    /* The first submitted pose has already consumed one fire key. */
    guard->current = &guard->fire;
    guard->key = 1;
    guard->repeats = 3;
    if (guard->current->count <= guard->key ||
        !bfm_forest_actor_pose_decode(guard->current->pose[guard->key], BFM_GUARD_POSE_BYTES, count, &guard->pose))
        goto failed;
    guard->draw = (BfmForestActor){guard->mesh, &guard->pose, &guard->root, (1u << count)-1u, NULL};
    return guard;
failed:
    bfm_forest_lesson_guard_free(guard);
    return NULL;
}

void bfm_forest_lesson_guard_free(BfmForestLessonGuard *guard)
{
    if (!guard) return;
    musashi_forest_player_mesh_free(guard->mesh);
    free(guard);
}

void bfm_forest_lesson_guard_tether(BfmForestLessonGuard *guard, int held, unsigned blade_yaw)
{
    if (!guard) return;
    guard->tethered = held != 0;
    if (held) {
        int16_t at[3];
        for (unsigned i = 0; i < 3; ++i) at[i] = (int16_t)guard->root.translation[i];
        bfm_forest_actor_root((blade_yaw + 2048) & 4095, at, &guard->root);
    }
}

int bfm_forest_lesson_guard_tick(BfmForestLessonGuard *guard)
{
    if (!guard || guard->tethered) return 1;
    if (++guard->key == guard->current->count) {
        guard->key = 0;
        if (guard->current == &guard->fire && !--guard->repeats) {
            guard->current = &guard->lower;
        } else if (guard->current == &guard->lower) {
            guard->current = &guard->idle;
        }
    }
    return bfm_forest_actor_pose_decode(guard->current->pose[guard->key], BFM_GUARD_POSE_BYTES,
                                       BFM_GUARD_PARTS, &guard->pose);
}

const BfmForestActor *bfm_forest_lesson_guard_actor(const BfmForestLessonGuard *guard)
{
    return guard ? &guard->draw : NULL;
}

int bfm_forest_lesson_guard_contact(const BfmForestLessonGuard *guard,
                                    const int16_t from[3], const int16_t to[3])
{
    int16_t center[3];
    return guard && from && to &&
           bfm_forest_guard_contact(&guard->box, &guard->root, from, to, center);
}

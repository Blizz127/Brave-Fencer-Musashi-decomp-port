/* SPDX-License-Identifier: MIT */
#include "forest_lift.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"

static int read_motion(const ForestPlayerArchive *archive, uint32_t table, unsigned slot,
                       int loop, BfmLiftMotion *motion)
{
    uint8_t pointer[4], prefix[12], entry[4];
    if (table > UINT32_MAX - 4 * slot ||
        !forest_player_archive_read(archive, table + 4 * slot, pointer, sizeof pointer))
        return 0;
    uint32_t script = u32le(pointer);
    if (script > UINT32_MAX - 256 || (script & 3) ||
        !forest_player_archive_read(archive, script, prefix, sizeof prefix) ||
        u32le(prefix) != 4 || u32le(prefix + 8) != 19)
        return 0;
    motion->event = BFM_LIFT_KEYS;
    for (unsigned word = 3; word < 64; ++word) {
        if (!forest_player_archive_read(archive, script + 4 * word, entry, sizeof entry)) return 0;
        uint32_t op = u32le(entry);
        if (op <= 1) return motion->count && op == (unsigned)loop;
        if (op == 7) {
            if (motion->event != BFM_LIFT_KEYS) return 0;
            motion->event = motion->count;
        } else if (op != 8) {
            if (op < 32 || (op & 3) || op > UINT32_MAX - 16 ||
                motion->count >= BFM_LIFT_KEYS) return 0;
            motion->frames[motion->count++] = op + 16;
        }
    }
    return 0;
}

int bfm_forest_lift_motions(const MusashiDiscMedia *disc, uint32_t table,
                            BfmForestLiftData *out)
{
    static const unsigned slots[BFM_LIFT_CLIPS] = {18, 25, 26, 27, 28};
    BfmForestLiftData data = {0};
    ForestPlayerArchive archive;
    if (!disc || !out || !forest_player_archive_open(disc, &archive)) return 0;
    for (unsigned clip = 0; clip < BFM_LIFT_CLIPS; ++clip) {
        int loop = clip != BFM_LIFT_RAISE && clip != BFM_LIFT_THROW;
        if (!read_motion(&archive, table, slots[clip], loop, &data.motion[clip])) return 0;
    }
    const BfmLiftMotion *raise = &data.motion[BFM_LIFT_RAISE];
    const BfmLiftMotion *toss = &data.motion[BFM_LIFT_THROW];
    if (raise->count < 2 || toss->event == 0 || toss->event >= toss->count) return 0;
    *out = data;
    return 1;
}

int bfm_forest_lift_load(const BfmForestField *field, const MusashiDiscMedia *disc,
                         uint32_t table, BfmForestLiftData *out)
{
    BfmForestLiftData data;
    if (!field || !out || !bfm_forest_lift_motions(disc, table, &data)) return 0;
    const BfmLiftMotion *raise = &data.motion[BFM_LIFT_RAISE];
    const BfmLiftMotion *toss = &data.motion[BFM_LIFT_THROW];
    data.raise_offsets = bfm_forest_room_bytes(field, UINT32_C(0x801893fc), 3 * raise->count);
    data.toss_offsets = bfm_forest_room_bytes(field, UINT32_C(0x80189438), 3 * toss->event);
    const uint8_t *velocity = bfm_forest_room_bytes(field, UINT32_C(0x8018917c), 12);
    const uint8_t *grab = bfm_forest_room_bytes(field, UINT32_C(0x80188b80) + 12 * 10, 4);
    if (!data.raise_offsets || !data.toss_offsets || !velocity || !grab) return 0;
    data.grab_base = u16le(grab) & 0xfff;
    data.grab_spread = u16le(grab + 2);
    data.walking_speed = (int32_t)u32le(velocity + 8);
    *out = data;
    return 1;
}

void bfm_forest_lift_world(const BfmForestTransform *root, const int16_t local[3],
                           int16_t world[3])
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t dot = 0;
        for (unsigned i = 0; i < 3; ++i)
            dot += (int64_t)root->rotation[axis][i] * local[i];
        world[axis] = (int16_t)(root->translation[axis] + dot / 4096 - (dot % 4096 < 0));
    }
}

int bfm_forest_lift_grip(const BfmForestLiftData *data, const BfmForestLift *lift,
                         const BfmForestTransform *root, int16_t world[3])
{
    const uint8_t *offset;
    if (!data || !lift || !root || !world || !lift->target) return 0;
    unsigned count = data->motion[BFM_LIFT_RAISE].count;
    if (count < 2) return 0;
    if (lift->clip == BFM_LIFT_THROW) {
        if (lift->age >= data->motion[BFM_LIFT_THROW].event) return 0;
        offset = data->toss_offsets + 3 * lift->age;
    } else if (lift->clip == BFM_LIFT_RAISE) {
        if (!lift->age || lift->age >= count) return 0;
        offset = data->raise_offsets + 3 * (lift->age - 1);
    } else if (lift->clip == BFM_LIFT_IDLE || lift->clip == BFM_LIFT_WALK) {
        offset = data->raise_offsets + 3 * (count - 2);
    } else return 0;
    int16_t local[3];
    for (unsigned axis = 0; axis < 3; ++axis)
        local[axis] = (int16_t)(lift->grip[axis] + (int8_t)offset[axis]);
    bfm_forest_lift_world(root, local, world);
    return 1;
}

BfmLiftStep bfm_forest_lift_tick(BfmForestLift *lift, const BfmForestLiftData *data,
                                 int direction, int square_edge)
{
    const BfmLiftMotion *motion = &data->motion[lift->clip];
    switch (lift->clip) {
    case BFM_LIFT_RAISE:
        if (++lift->age < motion->count) return BFM_LIFT_ATTACH;
        lift->clip = BFM_LIFT_IDLE;
        lift->age = 0;
        return BFM_LIFT_PLAY;
    case BFM_LIFT_IDLE:
    case BFM_LIFT_WALK: {
        if (!lift->target) return BFM_LIFT_FINISHED;
        if (square_edge) {
            lift->clip = BFM_LIFT_THROW;
            lift->age = 0;
            return BFM_LIFT_ATTACH;
        }
        BfmLiftClip next = direction ? BFM_LIFT_WALK : BFM_LIFT_IDLE;
        lift->age = next == lift->clip ? (lift->age + 1) % motion->count : 0;
        lift->clip = next;
        return direction ? BFM_LIFT_WALK_ATTACH : BFM_LIFT_ATTACH;
    }
    case BFM_LIFT_THROW:
        if (++lift->age >= motion->count) return BFM_LIFT_FINISHED;
        if (lift->age == motion->event) return BFM_LIFT_RELEASE;
        return lift->age < motion->event ? BFM_LIFT_ATTACH : BFM_LIFT_PLAY;
    default:
        return BFM_LIFT_FINISHED;
    }
}

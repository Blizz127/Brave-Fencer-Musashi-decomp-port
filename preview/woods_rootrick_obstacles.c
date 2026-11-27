/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_obstacles.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"

static const uint8_t *scene_record(const uint8_t *scene, size_t bytes,
                                   uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && count <= bytes - offset ? scene + offset : NULL;
}

static int mount_box(const uint8_t *scene, size_t bytes, uint32_t profile, BfmForestGuardBox *out)
{
    const uint8_t *model = scene_record(scene, bytes, profile, 12);
    if (!model || !u32le(model + 4)) return 0;
    const uint8_t *shape = scene_record(scene, bytes, u32le(model + 8), 16);
    if (!shape || u32le(shape) != 0) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        out->low[axis] = s16le(shape + 4 + 4 * axis);
        out->high[axis] = s16le(shape + 6 + 4 * axis);
        if (out->low[axis] > out->high[axis]) return 0;
    }
    return 1;
}

static int platform_mount(BfmRootrickPlatform *out, const uint8_t *row, uint16_t body_flags)
{
    if (u16le(row + 12) != 0) return 0;

    uint16_t flags = u16le(row + 8);
    if (flags != 0x8880 && flags != 0x0881) return 0;
    BfmRootrickPlatform platform = {0};
    for (unsigned axis = 0; axis < 3; ++axis)
        platform.xyz[axis] = s16le(row + 2 * axis);
    /* Both movers retract 256 whole units along negative Z. */
    if (platform.xyz[2] < INT16_MIN + 256) return 0;
    platform.flags = flags;
    platform.body_flags = body_flags;
    platform.signal = s16le(row + 14);
    platform.extended = 1;
    platform.phase = flags & 1 ? BFM_PLATFORM_READ_STORY : BFM_PLATFORM_CHOOSE_DWELL;
    *out = platform;
    return 1;
}

static void platform_race_entry(BfmRootrickPlatform *platform)
{
    if (platform->flags == 0x0881) platform->signal = 1;
    if (platform->flags == 0x8880) platform->flags &= (uint16_t)~0x0800u;
}

static void platform_race_hold(BfmRootrickPlatform *platform, int hold)
{
    if ((platform->flags & 15u) != 0) return;
    platform->signal = hold != 0;
    platform->flags |= 0x0800u;
}

static void platform_tick(BfmRootrickPlatform *platform, int story_158)
{
    switch (platform->phase) {
    case BFM_PLATFORM_CHOOSE_DWELL:
        if (platform->flags & 0x0800u)
            platform->timer = platform->extended ? 150 : 75;
        else
            platform->timer = platform->extended ? 75 : 180;
        platform->phase = BFM_PLATFORM_DWELL;
        break;
    case BFM_PLATFORM_DWELL:
        if (platform->timer) --platform->timer;
        if (platform->timer && !platform->signal) break;
        if (platform->signal && !platform->extended) {
            platform->phase = BFM_PLATFORM_HELD;
        } else {
            platform->extended = !platform->extended;
            platform->phase = platform->extended ? BFM_PLATFORM_EXTEND : BFM_PLATFORM_RETRACT;
            platform->timer = platform->extended ? 16 : 32;
            platform->body_flags |= 0x0400u;
        }
        break;
    case BFM_PLATFORM_HELD:
        if (!platform->signal) platform->phase = BFM_PLATFORM_DWELL;
        break;
    case BFM_PLATFORM_EXTEND:
        platform->xyz[2] += 16;
        if (--platform->timer == 0) platform->phase = BFM_PLATFORM_CHOOSE_DWELL;
        break;
    case BFM_PLATFORM_RETRACT:
        platform->xyz[2] -= 8;
        if (--platform->timer == 0) {
            platform->phase = BFM_PLATFORM_CHOOSE_DWELL;
            platform->body_flags &= (uint16_t)~0x0400u;
        }
        break;
    case BFM_PLATFORM_READ_STORY:
        if (story_158) platform->signal = 1;
        platform->phase = BFM_PLATFORM_WAIT_SIGNAL;
        break;
    case BFM_PLATFORM_WAIT_SIGNAL:
        if (platform->signal) {
            platform->timer = 32;
            platform->phase = BFM_PLATFORM_RETRACT_ONCE;
        }
        break;
    case BFM_PLATFORM_RETRACT_ONCE:
        platform->xyz[2] -= 8;
        if (--platform->timer == 0) platform->phase = BFM_PLATFORM_PARKED;
        break;
    case BFM_PLATFORM_PARKED:
        break;
    }
}

int bfm_rootrick_obstacles_mount(BfmRootrickObstacleField *field, const uint8_t *scene, size_t bytes)
{
    if (!field || !scene) return 0;
    BfmRootrickObstacleField mounted = {0};
    uint16_t body_flags[2];
    const uint8_t *room = scene_record(scene, bytes, UINT32_C(0x801961e8), 0xbc);
    if (!room || !mount_box(scene, bytes, UINT32_C(0x801950a4), &mounted.rival)) return 0;
    for (unsigned variant = 0; variant < 2; ++variant) {
        uint32_t profile = UINT32_C(0x80195834) + 52 * variant;
        const uint8_t *model = scene_record(scene, bytes, profile, 12);
        if (!model || !mount_box(scene, bytes, profile, &mounted.platform_box[variant])) return 0;
        body_flags[variant] = u16le(model + 2);
        mounted.model[variant] = u32le(model + 4);
        if (!scene_record(scene, bytes, mounted.model[variant], 16)) return 0;
    }

    uint32_t first = u32le(room + 0xb4), next = u32le(room + 0xb8);
    if (next <= first || (next - first) % 20 != 0) return 0;
    const uint8_t *rows = scene_record(scene, bytes, first, next - first);
    if (!rows) return 0;
    int has_rival = 0, ended = 0;
    for (size_t offset = 0; offset < next - first; offset += 20) {
        const uint8_t *row = rows + offset;
        unsigned kind = u16le(row + 6);
        if (!kind) {
            ended = 1;
            break;
        }
        if (kind == 128) has_rival = 1;
        if (kind == 176) {
            if (mounted.count >= BFM_ROOTRICK_PLATFORMS ||
                !platform_mount(&mounted.platform[mounted.count], row, body_flags[u16le(row + 8) & 1u])) return 0;
            ++mounted.count;
        }
    }
    if (!ended || !has_rival || mounted.count != BFM_ROOTRICK_PLATFORMS) return 0;
    *field = mounted;
    return 1;
}

void bfm_rootrick_obstacles_tick(BfmRootrickObstacleField *field, int story158)
{
    if (!field || field->count != BFM_ROOTRICK_PLATFORMS) return;
    for (unsigned i = 0; i < field->count; ++i)
        platform_tick(&field->platform[i], story158);
}

void bfm_rootrick_obstacles_challenge(BfmRootrickObstacleField *field)
{
    if (!field || field->count != BFM_ROOTRICK_PLATFORMS) return;
    for (unsigned i = 0; i < field->count; ++i)
        platform_race_entry(&field->platform[i]);
}

void bfm_rootrick_obstacles_hold(BfmRootrickObstacleField *field, int hold)
{
    if (!field || field->count != BFM_ROOTRICK_PLATFORMS) return;
    for (unsigned i = 0; i < field->count; ++i)
        platform_race_hold(&field->platform[i], hold);
}

int bfm_rootrick_obstacle_body(const BfmRootrickObstacleField *field, unsigned slot,
                               BfmForestGuardBox *shape, BfmForestTransform *root)
{
    if (!field || field->count != BFM_ROOTRICK_PLATFORMS || slot >= field->count || !shape || !root)
        return 0;
    const BfmRootrickPlatform *platform = &field->platform[slot];
    *shape = field->platform_box[platform->flags & 1u];
    bfm_forest_actor_root(0, platform->xyz, root);
    return 1;
}

uint16_t bfm_rootrick_obstacle_flags(const BfmRootrickObstacleField *field, unsigned slot)
{
    if (!field || field->count != BFM_ROOTRICK_PLATFORMS || slot >= field->count) return 0;
    return field->platform[slot].body_flags;
}

uint32_t bfm_rootrick_obstacle_model(const BfmRootrickObstacleField *field, unsigned slot)
{
    if (!field || field->count != BFM_ROOTRICK_PLATFORMS || slot >= field->count) return 0;
    return field->model[field->platform[slot].flags & 1u];
}

/* Both endpoints of each directed edge go through the rival's completed model
 * matrix. Integer face intersection need not give the same result in reverse. */
static int edges_touch(const BfmForestGuardBox *rival_box, const BfmForestTransform *rival,
                       const BfmForestGuardBox *platform_box, const BfmForestTransform *platform,
                       unsigned axes)
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (!(axes & (1u << axis))) continue;
        /* Toggle the two perpendicular coordinates in the body edge order. */
        unsigned first = axis == 1 ? 2 : 1;
        unsigned second = axis == 0 ? 2 : 0;
        for (unsigned corner = 0; corner < 4; ++corner) {
            int16_t local[3], start[3], end[3], center[3];
            local[first] = corner & 1u ? rival_box->high[first] : rival_box->low[first];
            local[second] = corner & 2u ? rival_box->high[second] : rival_box->low[second];
            local[axis] = rival_box->low[axis];
            bfm_forest_lift_world(rival, local, start);
            local[axis] = rival_box->high[axis];
            bfm_forest_lift_world(rival, local, end);
            if (bfm_forest_guard_contact(platform_box, platform, start, end, center)) return 1;
        }
    }
    return 0;
}

int bfm_rootrick_obstacle_crossing(const BfmRootrickObstacleField *field,
                                   const BfmForestTransform *proposed, const BfmForestTransform *previous,
                                   unsigned axis_mask)
{
    if (!field || field->count != BFM_ROOTRICK_PLATFORMS || !proposed || !previous || axis_mask > 7)
        return -1;
    for (unsigned i = 0; i < field->count; ++i) {
        BfmForestTransform root;
        BfmForestGuardBox shape;
        if (!bfm_rootrick_obstacle_body(field, i, &shape, &root)) return -1;
        if (edges_touch(&field->rival, proposed, &shape, &root, axis_mask)) {
            return edges_touch(&field->rival, previous, &shape, &root, 4) ? 2 : 1;
        }
    }
    return 0;
}

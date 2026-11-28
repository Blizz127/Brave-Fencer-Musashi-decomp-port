/* SPDX-License-Identifier: MIT */
#include "woods_3026_box_hit.h"
#include "forest_lift.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

static const uint8_t *scene_data(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && length <= bytes - offset ? scene + offset : NULL;
}

static int body_bounds(const uint8_t *scene, size_t bytes, const uint8_t *model,
                       BfmForestGuardBox *bounds)
{
    const uint8_t *shape = scene_data(scene, bytes, u32le(model + 8), 16);
    if (!shape || u32le(shape) != 0 || !(u16le(model + 2) & 0x1800)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        bounds->low[axis] = s16le(shape + 4 + 4 * axis);
        bounds->high[axis] = s16le(shape + 6 + 4 * axis);
        if (bounds->low[axis] > bounds->high[axis]) return 0;
    }
    return 1;
}

static int mount_box(const uint8_t *scene, size_t bytes, uint32_t room_address,
                     uint32_t profile_address, uint32_t segment_address, uint32_t destination,
                     BfmWoods3026BoxHit *out)
{
    if (!scene || !out) return 0;
    const uint8_t *room = scene_data(scene, bytes, room_address, 0xbc);
    const uint8_t *profiles = scene_data(scene, bytes, profile_address, 2 * 52);
    const uint8_t *segment = scene_data(scene, bytes, segment_address, 16);
    if (!room || !profiles || !segment) return 0;

    uint32_t first = u32le(room + 0xb4), next = u32le(room + 0xb8);
    if (next <= first || (next - first) % 20) return 0;
    size_t list_size = next - first;
    const uint8_t *list = scene_data(scene, bytes, first, list_size);
    if (!list) return 0;

    const uint8_t *box_row = NULL;
    for (size_t offset = 0; offset < list_size; offset += 20) {
        const uint8_t *row = list + offset;
        unsigned type = u16le(row + 6);
        if (type == 0) break;
        if (type == 483 && u16le(row + 8) == 0) {
            box_row = row;
            break;
        }
    }
    if (!box_row) return 0;

    BfmWoods3026BoxHit mounted;
    if (!body_bounds(scene, bytes, profiles, &mounted.bounds) ||
        !body_bounds(scene, bytes, profiles + 52, &mounted.lid_bounds)) return 0;
    mounted.destination = scene_data(scene, bytes, u32le(box_row + 16), 36);
    if (!mounted.destination || u32le(mounted.destination) != destination) return 0;
    int16_t position[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        mounted.use_segment[0][axis] = s16le(segment + 2 * axis);
        mounted.use_segment[1][axis] = s16le(segment + 8 + 2 * axis);
        position[axis] = s16le(box_row + 2 * axis);
    }
    mounted.yaw = u16le(box_row + 12) & 4095;
    bfm_forest_actor_root(mounted.yaw, position, &mounted.root);
    bfm_forest_actor_root((mounted.yaw + 2048) & 4095, position, &mounted.lid_root);
    *out = mounted;
    return 1;
}

int bfm_woods_3026_box_hit_mount(const uint8_t *scene, size_t bytes, BfmWoods3026BoxHit *out)
{
    return mount_box(scene, bytes, UINT32_C(0x801961e8), UINT32_C(0x801e8554), UINT32_C(0x801e8670),
                     UINT32_C(0x00ff3026), out);
}

int bfm_woods_3026_box_hit_mount_shaft(const uint8_t *scene, size_t bytes, BfmWoods3026BoxHit *out)
{
    return mount_box(scene, bytes, UINT32_C(0x80181f28), UINT32_C(0x80197174), UINT32_C(0x80197290),
                     UINT32_C(0x00ff3045), out);
}

int bfm_woods_3026_box_can_use(const BfmWoods3026BoxHit *box, const BfmFieldInput *input,
                              const BfmForestTransform *player_model, const int16_t player_xyz[3],
                              uint32_t player_flags, int interaction_busy)
{
    if (!box || !box->destination || !input || !input->square_pressed ||
        !player_model || !player_xyz || interaction_busy || (player_flags & 0x404)) return 0;

    int16_t start[3], end[3], center[3];
    bfm_forest_lift_world(player_model, box->use_segment[0], start);
    bfm_forest_lift_world(player_model, box->use_segment[1], end);
    if (!bfm_forest_guard_contact(&box->bounds, &box->root, start, end, center)) return 0;

    int dz = box->root.translation[2] - player_xyz[2];
    int dx = player_xyz[0] - box->root.translation[0];
    unsigned angle = (bfm_forest_shore_bearing(dz, dx) - 1024u - box->yaw) & 4095;
    return angle < 768 || angle > 4096 - 768;
}

static int slide_part(const BfmForestGuardBox *bounds, const BfmForestTransform *root,
                      unsigned tick, const int16_t start[3], int16_t end[3], int16_t actor[3])
{
    int16_t center[3], hit[3], normal[3];
    if (!bfm_forest_guard_contact_point(bounds, root, start, end, center, hit, normal)) return 0;
    int outside = bfm_forest_body_slide_on_entry(bounds, root, start, end, hit);
    /* Other parts see the swept endpoint. The lateral shove applies to the
     * player and becomes the proposed position on the next sweep pass. */
    memcpy(end, hit, sizeof hit);
    if (!outside || normal[1] > 2048 || normal[1] < -2048) {
        unsigned phase = (tick >> 5) & 3u;
        int dx = phase < 2 ? 7 : 8;
        int dz = phase == 2 ? 7 : 8;
        hit[0] += hit[0] > center[0] ? dx : -dx;
        hit[2] += hit[2] > center[2] ? dz : -dz;
    }
    memcpy(actor, hit, sizeof hit);
    return 1;
}

int bfm_woods_3026_box_body(const BfmWoods3026BoxHit *box, unsigned field_tick,
                           const int16_t previous[3], int16_t requested[3])
{
    if (!box || !box->destination || !previous || !requested) return 0;
    int16_t start[3] = { previous[0], previous[1] - 16, previous[2] };
    int16_t actor[3] = { requested[0], requested[1] - 16, requested[2] };
    int blocked = 0;
    for (unsigned pass = 0; pass < 2; ++pass) {
        int16_t end[3];
        memcpy(end, actor, sizeof end);
        blocked |= slide_part(&box->bounds, &box->root, field_tick, start, end, actor);
        blocked |= slide_part(&box->lid_bounds, &box->lid_root, field_tick, start, end, actor);
    }
    if (blocked) {
        actor[1] += 16;
        memcpy(requested, actor, sizeof actor);
    }
    return blocked;
}

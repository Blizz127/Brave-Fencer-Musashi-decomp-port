/* SPDX-License-Identifier: MIT */
#include "valley_hinge.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

static const uint8_t *mounted(const uint8_t *room, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base || count > bytes || address - base > bytes - count) return NULL;
    return room + (address - base);
}

static int read_box(BfmForestGuardBox *box, const uint8_t *scene, size_t bytes, uint32_t address)
{
    const uint8_t *row = mounted(scene, bytes, address, 16);
    if (!row || u32le(row)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        box->low[axis] = s16le(row + 4 + 4 * axis);
        box->high[axis] = s16le(row + 6 + 4 * axis);
        if (box->low[axis] >= box->high[axis]) return 0;
    }
    return 1;
}

static void update_transform(BfmValleyHinge *h)
{
    int down = h->phase == BFM_VALLEY_HINGE_DOWN;
    bfm_forest_actor_pitch_yaw(h->pitch, h->yaw, down ? h->down_pivot : h->pivot, &h->body);
    h->model = h->body;
    if (down) {
        int16_t part_origin[3];
        bfm_forest_lift_world(&h->body, h->down_part, part_origin);
        for (unsigned i = 0; i < 3; ++i) h->model.translation[i] = part_origin[i];
    }
}

int bfm_valley_hinge_load(BfmValleyHinge *h, const uint8_t *scene,
                         size_t bytes, const uint8_t flags[64])
{
    if (!h || !scene || !flags) return 0;
    const uint8_t *placement = mounted(scene, bytes, UINT32_C(0x80186a58), 20);
    const uint8_t *profile = mounted(scene, bytes, UINT32_C(0x80186068), 12);
    const uint8_t *flag = mounted(scene, bytes, UINT32_C(0x801860c0), 4);
    const uint8_t *part = mounted(scene, bytes, UINT32_C(0x801860f4), 12);
    const uint8_t *points = mounted(scene, bytes, UINT32_C(0x80186184), 16);
    if (!placement || u16le(placement + 6) != 228 || u16le(placement + 8) != 0x5002 ||
        !profile || !flag || u32le(flag) >= 512 || !part || !points) return 0;
    BfmValleyHinge loaded = {.flag = u32le(flag), .yaw = u16le(placement + 12)};
    if (!read_box(&loaded.raised_box, scene, bytes, u32le(profile + 8)) ||
        !read_box(&loaded.down_box, scene, bytes, UINT32_C(0x80185ff0))) return 0;
    int16_t rebase[3];
    for (unsigned i = 0; i < 3; ++i) {
        loaded.pivot[i] = s16le(placement + 2 * i);
        loaded.down_part[i] = s16le(part + 2 * i);
        loaded.strike_point[i] = s16le(points + 2 * i);
        rebase[i] = s16le(points + 8 + 2 * i);
    }
    /* The final solid is measured from the other end. Its part offset keeps
     * the stone in place through that change, both live and on a saved return. */
    BfmForestTransform tilt;
    bfm_forest_actor_pitch_yaw(4096 - 768, 0, loaded.pivot, &tilt);
    bfm_forest_lift_world(&tilt, rebase, loaded.down_pivot);
    if (flags[loaded.flag / 8] & (1u << (loaded.flag % 8))) {
        loaded.phase = BFM_VALLEY_HINGE_DOWN;
        loaded.pitch = 1280;
    } else {
        loaded.pitch = u16le(placement + 10);
    }
    update_transform(&loaded);
    *h = loaded;
    return 1;
}

static const BfmForestGuardBox *shape(const BfmValleyHinge *h)
{
    return h->phase == BFM_VALLEY_HINGE_DOWN ? &h->down_box : &h->raised_box;
}

int bfm_valley_hinge_contact(const BfmValleyHinge *h, const int16_t from[3],
                             const int16_t to[3], int16_t hit[3], int16_t normal[3])
{
    if (!h) return 0;
    int16_t center[3];
    return bfm_forest_guard_contact_point(shape(h), &h->body, from, to, center, hit, normal);
}

int bfm_valley_hinge_slide(const BfmValleyHinge *h, const int16_t from[3],
                           const int16_t to[3], int16_t resolved[3])
{
    return h && bfm_forest_body_slide(shape(h), &h->body, from, to, resolved);
}

int bfm_valley_hinge_block_body(const BfmValleyHinge *h, const int16_t previous[3],
                                int16_t proposed[3], unsigned field_tick, int airborne)
{
    return h && bfm_valley_box_block_body(shape(h), &h->body, previous, proposed, field_tick, airborne);
}

int bfm_valley_box_block_body(const BfmForestGuardBox *box, const BfmForestTransform *body,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne)
{
    if (!box || !body || !previous || !proposed) return 0;
    int16_t start[3] = {previous[0], (int16_t)(previous[1] - 16), previous[2]};
    unsigned quarter = (field_tick >> 5) & 3;
    int push_x = quarter < 2 ? 7 : 8, push_z = quarter == 2 ? 7 : 8;
    int touched = 0;
    for (unsigned pass = 0; pass < (airborne ? 1u : 2u); ++pass) {
        int16_t end[3] = {proposed[0], (int16_t)(proposed[1] - 16), proposed[2]};
        int16_t center[3], normal[3], resolved[3];
        if (!bfm_forest_guard_contact_point(box, body, start, end, center, resolved, normal))
            break;
        touched = 1;
        int entered = bfm_forest_body_slide_on_entry(box, body, start, end, resolved);
        if (entered) {
            proposed[0] = resolved[0];
            proposed[1] = (int16_t)(resolved[1] + 16);
            proposed[2] = resolved[2];
        }
        if (!entered || normal[1] < -2048 || normal[1] > 2048) {
            const int16_t *side = entered || airborne ? resolved : start;
            proposed[0] = (int16_t)(resolved[0] + (center[0] >= side[0] ? -push_x : push_x));
            proposed[2] = (int16_t)(resolved[2] + (center[2] >= side[2] ? -push_z : push_z));
        }
    }
    return touched;
}

BfmValleyHingeEvent bfm_valley_hinge_step(BfmValleyHinge *h, int quake_in_view, const int16_t foot[3])
{
    BfmValleyHingeEvent event = {0};
    if (!h || h->phase == BFM_VALLEY_HINGE_DOWN) return event;
    if (h->phase == BFM_VALLEY_HINGE_RAISED) {
        if (quake_in_view) h->phase = BFM_VALLEY_HINGE_TIPPING;
        return event;
    }
    h->angular_speed += 4 * 65536;
    int delta = h->angular_speed / 65536 - (h->angular_speed % 65536 < 0);
    int next_pitch = (int)h->pitch + delta;
    if (next_pitch >= 1280) {
        event.impact = 1;
        /* The impact uses the actor transform already present this field,
         * before the clamped pitch or the new pivot is installed. */
        if (foot && foot[1] >= INT16_MIN + 80) {
            int16_t head[3] = {foot[0], (int16_t)(foot[1] - 80), foot[2]};
            event.player_hit = bfm_valley_hinge_contact(h, foot, head, event.hit_position, event.hit_normal);
        }
        bfm_forest_lift_world(&h->body, h->strike_point, event.debris_origin);
        next_pitch = 1280;
        h->angular_speed = -(h->angular_speed / 4);
        if (++h->impacts == 3) {
            h->phase = BFM_VALLEY_HINGE_DOWN;
            event.settled = 1;
        }
    }
    h->pitch = (unsigned)next_pitch;
    update_transform(h);
    return event;
}

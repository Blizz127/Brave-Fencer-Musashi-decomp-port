/* SPDX-License-Identifier: MIT */
#include "village_mine_shaft_fan.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { PLACEMENT_BYTES = 20, QUIET_FIELDS = 64, SPIN_FIELDS = 192 };
static const uint32_t scene_base = UINT32_C(0x80128158);

struct BfmMineShaftFan {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
    BfmForestGuardBox hub, arm;
    BfmForestTransform piece[BFM_MINE_SHAFT_FAN_PIECES];
    int16_t center[3];
    uint16_t heading;
    int speed, spinning, timer;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (!scene || address < scene_base || count > bytes || address - scene_base > bytes - count)
        return NULL;
    return scene + (address - scene_base);
}

static int read_box(const uint8_t *data, BfmForestGuardBox *box)
{
    if (!data || u32le(data)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        box->low[axis] = s16le(data + 4 + 4*axis);
        box->high[axis] = s16le(data + 6 + 4*axis);
        if (box->low[axis] >= box->high[axis]) return 0;
    }
    return 1;
}

static void piece_root(const BfmMineShaftFan *fan, unsigned piece, BfmForestTransform *root)
{
    int16_t yaw = (int16_t)(fan->heading + (piece ? (piece-1)*1024 : 0));
    bfm_forest_actor_root((unsigned)abs(yaw), fan->center, root);
    if (yaw < 0) {
        root->rotation[0][2] = -root->rotation[0][2];
        root->rotation[2][0] = -root->rotation[2][0];
    }
}

BfmMineShaftFan *bfm_mine_shaft_fan_open(const uint8_t *scene, size_t bytes)
{
    const uint8_t *binding = mounted(scene, bytes, UINT32_C(0x8017e300), 4);
    if (!binding || u32le(binding) != UINT32_C(0x80181f28)) return NULL;
    const uint8_t *room = mounted(scene, bytes, u32le(binding), 0xbc);
    if (!room) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end-first) % PLACEMENT_BYTES) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end-first);
    if (!rows) return NULL;
    const uint8_t *placement = NULL;
    for (size_t offset = 0; offset < end-first; offset += PLACEMENT_BYTES) {
        const uint8_t *row = rows + offset;
        unsigned kind = u16le(row + 6);
        if (!kind) break;
        if (kind != 400) continue;
        if (placement || u16le(row + 8)) return NULL;
        placement = row;
    }
    if (!placement) return NULL;
    BfmMineShaftFan *fan = calloc(1, sizeof *fan);
    if (!fan) return NULL;
    unsigned parts = 0;
    fan->mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_base, u32le(placement+16), &parts);
    if (!fan->mesh || parts != 1 ||
        !read_box(mounted(scene, bytes, UINT32_C(0x80181e54), 16), &fan->hub) ||
        !read_box(mounted(scene, bytes, UINT32_C(0x80181e74), 16), &fan->arm)) {
        bfm_mine_shaft_fan_free(fan);
        return NULL;
    }
    for (unsigned axis = 0; axis < 3; ++axis) fan->center[axis] = s16le(placement + 2*axis);
    fan->heading = u16le(placement + 12);
    fan->timer = QUIET_FIELDS;
    for (unsigned i = 0; i < BFM_MINE_SHAFT_FAN_PIECES; ++i)
        piece_root(fan, i, &fan->piece[i]);
    bfm_forest_actor_root(0, (const int16_t[3]){0}, &fan->pose.local[0]);
    return fan;
}

void bfm_mine_shaft_fan_free(BfmMineShaftFan *fan)
{
    if (fan) {
        musashi_forest_player_mesh_free(fan->mesh);
        free(fan);
    }
}

const BfmForestTransform *bfm_mine_shaft_fan_shape(const BfmMineShaftFan *fan,
    unsigned piece, const BfmForestGuardBox **box)
{
    if (!fan || piece >= BFM_MINE_SHAFT_FAN_PIECES) return NULL;
    if (box) *box = piece ? &fan->arm : &fan->hub;
    return &fan->piece[piece];
}

int bfm_mine_shaft_fan_body(const BfmMineShaftFan *fan, int rider,
    const int16_t previous[3], int16_t foot[3], unsigned tick, int mode)
{
    if (!fan || !previous || !foot) return 0;
    int contacts = 0;
    int16_t start[3] = {previous[0], previous[1]-16, previous[2]};
    unsigned phase = (tick >> 5) & 3;
    int push_x = phase < 2 ? 7 : 8, push_z = phase == 2 ? 7 : 8;
    int passes = mode == BFM_SHAFT_BODY_GROUND ? 2 : 1;
    while (passes--) {
        int16_t endpoint[3] = {foot[0], foot[1]-16, foot[2]};
        for (int i = 0; i < BFM_MINE_SHAFT_FAN_PIECES; ++i) {
            if (i == rider) continue;
            const BfmForestGuardBox *box = i ? &fan->arm : &fan->hub;
            const BfmForestTransform *root = &fan->piece[i];
            int16_t center[3], hit[3], normal[3];
            if (!bfm_forest_guard_contact_point(box,root,start,endpoint,center,hit,normal))
                continue;
            int entered = bfm_forest_body_slide_on_entry(box,root,start,endpoint,hit);
            memcpy(endpoint,hit,sizeof endpoint);
            if (entered) {
                contacts |= 1;
                foot[0] = hit[0];
                foot[1] = hit[1] + 16;
                foot[2] = hit[2];
            } else if (mode == BFM_SHAFT_BODY_GROUND) {
                contacts |= 2;
            } else if (mode == BFM_SHAFT_BODY_AIR_NO_FACE_PUSH) {
                contacts = 1;
            }
            if (!entered || (mode != BFM_SHAFT_BODY_AIR_NO_FACE_PUSH && abs(normal[1]) > 2048)) {
                const int16_t *side = !entered && mode == BFM_SHAFT_BODY_GROUND ? start : hit;
                foot[0] = hit[0] + (center[0] >= side[0] ? -push_x : push_x);
                foot[2] = hit[2] + (center[2] >= side[2] ? -push_z : push_z);
            }
        }
    }
    return contacts;
}

BfmMineBasketContact bfm_mine_shaft_fan_probe(const BfmMineShaftFan *fan,
    const int16_t previous[3], const int16_t candidate[3], const int16_t last_supported[3],
    int *rider, int16_t foot[3])
{
    if (!candidate || !foot || !rider) return BFM_MINE_BASKET_MISS;
    int held = *rider;
    *rider = -1;
    memmove(foot,candidate,3*sizeof *foot);
    if (!fan || !previous || candidate[1] > fan->center[1] ||
        held < -1 || held >= BFM_MINE_SHAFT_FAN_PIECES || (held >= 0 && !last_supported))
        return BFM_MINE_BASKET_MISS;
    int first = held >= 0 ? held : 0;
    int end = held >= 0 ? held+1 : BFM_MINE_SHAFT_FAN_PIECES;
    for (int i = first; i < end; ++i) {
        const BfmForestGuardBox *box = i ? &fan->arm : &fan->hub;
        BfmMineBasketContact result = bfm_mine_basket_probe(box,&fan->piece[i],
            previous,candidate,held >= 0 ? last_supported : NULL,foot);
        if (result != BFM_MINE_BASKET_MISS) {
            if (result != BFM_MINE_BASKET_TOUCH) *rider = i;
            return result;
        }
    }
    return BFM_MINE_BASKET_MISS;
}

void bfm_mine_shaft_fan_shadow(const BfmMineShaftFan *fan,
    const int16_t carried[3], BfmBedroomRoot *shadow)
{
    if (!fan || !carried || !shadow) return;
    int16_t start[3] = {carried[0], carried[1]-16, carried[2]};
    int16_t end[3] = {carried[0], carried[1]+752, carried[2]};
    for (int i = 0; i < BFM_MINE_SHAFT_FAN_PIECES; ++i) {
        const BfmForestGuardBox *box = i ? &fan->arm : &fan->hub;
        int16_t center[3], hit[3], normal[3];
        if (!bfm_forest_guard_contact_point(box,&fan->piece[i],start,end,center,hit,normal))
            continue;
        if (shadow->shadow_visible && hit[1] >= shadow->shadow_xyz[1]) continue;
        memcpy(shadow->shadow_xyz,hit,sizeof hit);
        memcpy(shadow->shadow_normal,normal,sizeof normal);
        shadow->shadow_visible = 1;
    }
}

static void change_speed(BfmMineShaftFan *fan, BfmMineShaftFanFrame *frame)
{
    if (fan->timer > 0) --fan->timer;
    if (fan->spinning) {
        if (!fan->timer) {
            fan->speed -= 2;
            if (fan->speed < 16) {
                fan->spinning = 0;
                fan->timer = QUIET_FIELDS;
            }
        } else {
            if (fan->speed < 256) fan->speed += 4;
            if (fan->timer == 1) {
                frame->sound_count = 2;
                frame->sound[0].command = 0x721;
                frame->sound[1].command = 4;
                frame->sound[1].value = 0x720;
            }
        }
    } else if (!fan->timer) {
        fan->speed += 4;
        if (fan->speed > 16) {
            fan->spinning = 1;
            fan->timer = SPIN_FIELDS;
        }
    } else {
        if (fan->speed > 2) fan->speed -= 2;
        else if (((fan->heading + 2) & 1023) < 4) fan->speed = 0;
        if (fan->timer == 1) {
            frame->sound_count = 1;
            frame->sound[0].command = 0x720;
        }
    }
    fan->heading = (uint16_t)(fan->heading + fan->speed);
}

/* Both orientations are level, with a common center. Round each rotation
 * separately, in the unit of the input (Q16 for a rider, integer otherwise). */
static void rotate_foot(const BfmForestTransform *before, const BfmForestTransform *after,
                        int32_t relative_x, int32_t relative_z, int32_t *x, int32_t *z)
{
    int32_t local_x = (int32_t)(((int64_t)before->rotation[0][0] * relative_x +
                                 (int64_t)before->rotation[2][0] * relative_z) >> 12);
    int32_t local_z = (int32_t)(((int64_t)before->rotation[0][2] * relative_x +
                                 (int64_t)before->rotation[2][2] * relative_z) >> 12);
    *x = (int32_t)(((int64_t)after->rotation[0][0] * local_x +
                    (int64_t)after->rotation[0][2] * local_z) >> 12);
    *z = (int32_t)(((int64_t)after->rotation[2][0] * local_x +
                    (int64_t)after->rotation[2][2] * local_z) >> 12);
}

static void move_piece(BfmMineShaftFan *fan, unsigned i, int ridden, int32_t foot[3])
{
    BfmForestTransform before = fan->piece[i];
    BfmForestTransform *after = &fan->piece[i];
    piece_root(fan, i, after);
    if (!foot) return;
    int32_t x, z;
    if (ridden) {
        int32_t cx = fan->center[0] * 65536, cz = fan->center[2] * 65536;
        rotate_foot(&before, after, foot[0] - cx, foot[2] - cz, &x, &z);
        foot[0] = cx + x;
        foot[2] = cz + z;
    } else {
        int16_t end[3] = {foot[0] >> 16, foot[1] >> 16, foot[2] >> 16};
        rotate_foot(&before, after, end[0] - fan->center[0], end[2] - fan->center[2], &x, &z);
        int16_t start[3] = {x + fan->center[0], end[1], z + fan->center[2]};
        int16_t center[3], hit[3];
        if (bfm_forest_guard_contact_point(i ? &fan->arm : &fan->hub, after,
                                          start, end, center, hit, NULL)) {
            for (unsigned axis = 0; axis < 3; ++axis)
                foot[axis] = hit[axis] * 65536 + (int)((uint32_t)foot[axis] & 0xffffu);
        }
    }
}

static void apply_wind(const BfmMineShaftFan *fan, unsigned action, int32_t foot[3], int32_t *vertical)
{
    if (!fan->speed || !foot || !vertical) return;
    int32_t dx = (foot[0] >> 16) - fan->center[0], dz = (foot[2] >> 16) - fan->center[2];
    int height = fan->center[1] - (foot[1] >> 16);
    if (height < -8 || height > 448 || (int64_t)dx*dx + (int64_t)dz*dz > 496*496) return;
    if (action != 2 && action != 3) {
        foot[1] -= 4 * 65536;
        *vertical = 0;
    }
    *vertical -= (fan->speed < 128 ? fan->speed : 128) * 2048;
}

BfmMineShaftFanFrame bfm_mine_shaft_fan_step(BfmMineShaftFan *fan, unsigned player_action,
    int rider, int32_t foot_q16[3], int32_t *vertical_q16)
{
    BfmMineShaftFanFrame frame = {0};
    if (!fan) return frame;
    change_speed(fan, &frame);
    move_piece(fan, 0, rider == 0, foot_q16);
    apply_wind(fan, player_action, foot_q16, vertical_q16);
    for (unsigned i = 1; i < BFM_MINE_SHAFT_FAN_PIECES; ++i)
        move_piece(fan, i, rider == (int)i, foot_q16);
    if (rider >= 0 && rider < BFM_MINE_SHAFT_FAN_PIECES) frame.carry_yaw = fan->speed;
    return frame;
}

int bfm_mine_shaft_fan_draw(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const BfmMineShaftFan *fan = context;
    if (!fan || !figure || !view) return 0;
    BfmForestActor actor = {.mesh = fan->mesh, .pose = &fan->pose, .root = &fan->piece[0], .visible_parts = 1};
    return bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0);
}

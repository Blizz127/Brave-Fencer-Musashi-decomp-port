/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_field.h"
#include "field_waypoint.h"
#include "village_talk_camera.h"
#include "woods_rootrick_notice.h"
#include "woods_rootrick_impact.h"
#include "woods_twinpeak_stride.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned kind;
    int x, z;
} PlayerOrder;

struct BfmEarthShrineField {
    const uint8_t *scene, *talk_packets;
    size_t scene_bytes;
    unsigned talk_count, talk_key, talking, talk_fusion;
    /* The longest uninterrupted group on the mounted tape is a pair. */
    PlayerOrder orders[2];
    unsigned count, entered, walking, ready, started, window;
    int hud;
    BfmForestMarker notice;
    BfmWoodsRootrickImpact shake;
    BfmWoodsBoxView *shake_view;
    const BfmEarthShrineRelease *child;
    int16_t primary_xyz[3], primary_scale_y;
    int primary_sampled;
    BfmVillageTalkCamera shot;
    BfmVillageCamera saved, presented, tail;
    unsigned saved_camera, returning, tail_fields;
};

static const uint32_t main_base = UINT32_C(0x800cedf8);

BfmEarthShrineField *bfm_earth_shrine_field_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes) {
    /* This room's queued talking action selects the looping MAIN/10 body.
     * Its header selects a model and sheathes Lumina. Each following packet
     * lasts one animation field until the restart command. */
    size_t offset = UINT32_C(0x800d47f4) - main_base;
    if (!scene || !main || offset > main_bytes || main_bytes - offset < 16) return NULL;
    const uint8_t *clip = main + offset;
    if (u32le(clip) != 4 || u32le(clip + 8) != 19) return NULL;
    size_t slots = (main_bytes - offset - 12) / 4;
    unsigned n = 0;
    while (n < slots && n < 256 && u32le(clip + 12 + 4*n) != 1) {
        uint32_t packet = u32le(clip + 12 + 4*n);
        if (packet < main_base || packet - main_base > main_bytes ||
            main_bytes - (packet - main_base) < 256) return NULL;
        ++n;
    }
    if (!n || n == slots || n == 256) return NULL;
    BfmEarthShrineField *f = calloc(1, sizeof *f);
    if (f) {
        f->talk_packets = clip + 12;
        f->talk_count = n;
        f->scene = scene;
        f->scene_bytes = bytes;
    }
    return f;
}

void bfm_earth_shrine_field_free(BfmEarthShrineField *f) { free(f); }

static int queue(BfmEarthShrineField *f, unsigned op, int x, int z) {
    if (f->count == sizeof f->orders / sizeof f->orders[0]) return 0;
    f->orders[f->count++] = (PlayerOrder){op, x, z};
    return 1;
}

static void pack_station(uint8_t station[8], const int16_t xyz[3]) {
    for (unsigned i = 0; i < 3; ++i) {
        uint16_t v = (uint16_t)xyz[i];
        station[2*i] = (uint8_t)v;
        station[2*i+1] = (uint8_t)(v >> 8);
    }
    station[6] = station[7] = 0;
}

int bfm_earth_shrine_field_shake(BfmEarthShrineField *f, const uint8_t *record, size_t bytes) {
    if (!f || !record) return 0;
    /* The field service advances an eight-bit pair index. Validate its
     * terminator before replacing a track which may already be playing. */
    for (size_t i = 0; i < bytes / 2 && i < 256; ++i) {
        if (!record[2*i+1]) {
            f->shake.shake = record;
            bfm_woods_rootrick_impact_shake(&f->shake);
            return 1;
        }
    }
    return 0;
}

int bfm_earth_shrine_field_event(BfmEarthShrineField *f, const BfmEarthShrineRelease *s,
    BfmEarthReleaseEvent event, unsigned kind, uint32_t operand, const uint8_t *data,
    size_t bytes, BfmWoodsBoxView *view, BfmForestMessage **opened) {
    if (!f || !s) return 0;
    f->child = s;
    switch (event) {
    case BFM_EARTH_RELEASE_SHAKE: return bfm_earth_shrine_field_shake(f, data, bytes);
    case BFM_EARTH_PLAYER_READY: return f->ready && !f->count;
    case BFM_EARTH_MESSAGE_CLOSED: return !f->window;
    case BFM_EARTH_CAMERA_READY: return !f->shot.phase;
    case BFM_EARTH_FIELD_COMMAND: break;
    default: return 0;
    }
    switch (kind) {
    case 0x4b:
        /* The shared entry wrapper waits for the previous player action
         * before dispatching the first command, after this child's shake. */
        if (!s->shake_ticks || !f->ready) return 0;
        f->started = 1;
        f->hud = -1;
        return 1;
    case 0x4a: f->hud = 1; return 1;
    case 0x04:
        /* The 272 draw root is upright. Both authored targets resolve once
         * at command admission; the child continues moving afterwards. */
        return queue(f, kind, s->xyz[0] + (int16_t)operand,
                     s->xyz[2] + (int16_t)(operand >> 16));
    case 0x0a: return queue(f, kind, s->xyz[0], s->xyz[2]);
    case 0x07: return queue(f, kind, (int16_t)operand, 0);
    case 0x19: return operand == 2 && queue(f, kind, 0, 0);
    case 0x1d: case 0x46: case 0x47: return queue(f, kind, 0, 0);
    case 0x57: {
        const BfmVillageCamera *camera = bfm_woods_box_view_camera(view);
        if (!camera) return 0;
        f->presented = f->saved = *camera;
        f->saved_camera = 1;
        return 1;
    }
    case 0x21: {
        if (!f->saved_camera || !data || bytes != 16) return 0;
        /* Both shots aim along local Y. The second samples the primary's
         * last submitted matrix: the hidden final shrink tick never wrote
         * a zero-scale transform. Its local offsets still use that scale. */
        if (s16le(data) || s16le(data + 4)) return 0;
        int16_t origin[3];
        memcpy(origin, s->xyz, sizeof origin);
        if (s->stage >= BFM_EARTH_RISING) {
            if (!f->primary_sampled) return 0;
            memcpy(origin, f->primary_xyz, sizeof origin);
            int local_y = s16le(data + 2);
            int scaled_y = local_y * f->primary_scale_y;
            origin[1] += scaled_y / 4096 - (scaled_y % 4096 < 0) - local_y;
        }
        uint8_t station[8];
        pack_station(station, origin);
        if (!bfm_village_talk_camera_frame(&f->shot, station, data, &f->presented)) return 0;
        memcpy(f->shot.entry_eye, f->saved.eye, sizeof f->shot.entry_eye);
        memcpy(f->shot.entry_target, f->saved.target, sizeof f->shot.entry_target);
        return 1;
    }
    case 0x58: return f->saved_camera;
    case 0x20:
        if (!f->saved_camera) return 0;
        bfm_village_talk_camera_release(&f->shot);
        f->returning = 1;
        return 1;
    case 0: case 1:
        if (!opened || *opened || f->window || !data || !bytes || data[0] != 5) return 0;
        *opened = bfm_field_message_borrow(data, bytes);
        if (!*opened) return 0;
        f->window = 1;
        return 1;
    default: return 0;
    }
}

int bfm_earth_shrine_field_closed(BfmEarthShrineField *f) {
    if (!f || !f->window) return 0;
    f->window = 0;
    return 1;
}

int bfm_earth_shrine_field_take_hud(BfmEarthShrineField *f) {
    if (!f) return 0;
    int request = f->hud;
    f->hud = 0;
    return request;
}

/* The lip of the shrine is a bevel over a walkable floor. First project the
 * candidate through the support walls, then use its actual floor and normal. */
static int walk(BfmEarthShrineField *f, BfmVillageMotion *motion, BfmBedroomRoot *root,
                const PlayerOrder *order, int yaw, int32_t position[3], int *done) {
    const BfmVillageGround *ground = bfm_village_motion_ground(motion);
    int dx = order->x - root->xyz[0], dz = order->z - root->xyz[2];
    *done = (int64_t)dx*dx + (int64_t)dz*dz < 16*16;
    int16_t to[3] = {(int16_t)order->x, root->xyz[1], (int16_t)order->z};
    if (!*done) {
        BfmWaypointInput stick = bfm_waypoint_input(dx, dz, 0, yaw);
        root->yaw = (unsigned)stick.yaw;
        if (!bfm_twinpeak_stride_target(root, position, stick.speed_q16, root->yaw, to)) return 0;
    }
    uint16_t material;
    if (!(bfm_village_ground_support_probe(ground, 0, root->xyz, to, &material) & 0x6000))
        return 0;
    int16_t from[3];
    memcpy(from, to, sizeof from);
    BfmForestContact floor;
    if (bfm_village_ground_step(ground, from, to, &floor) != BFM_VILLAGE_GROUND_FOOTING) return 0;
    memcpy(root->xyz, to, sizeof root->xyz);
    memcpy(root->normal, floor.normal, sizeof root->normal);
    if (*done && f->walking) root->locomotion_edge = BFM_TRAVEL_STAND;
    if (!*done && !f->walking) root->locomotion_edge = BFM_TRAVEL_WALK;
    f->walking = !*done;
    return 1;
}

int bfm_earth_shrine_field_player(BfmEarthShrineField *f, BfmVillageMotion *motion,
    BfmWoodsBoxView *view, struct BfmGameplayRun *run, int player_ready) {
    if (!f || !motion || !view) return 0;
    /* The stock queue reports busy for the entire visit that dequeues the
     * last order, even if the requested action finishes immediately. */
    f->ready = !!player_ready && !f->count;
    if (!f->started) return 1;
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    const BfmVillageCamera *camera = bfm_woods_box_view_camera(view);
    if (!player || !camera) return 0;
    bfm_forest_marker_advance(&f->notice, player->root.xyz);
    if (!player_ready) return 1;
    BfmBedroomRoot root = player->root;
    root.locomotion_edge = BFM_TRAVEL_NONE;
    int yaw = bfm_village_motion_view_yaw(camera->eye, camera->target);
    int32_t position[3];
    if (!bfm_village_motion_position_q16(motion, position)) return 0;
    int done = 0, handled_motion = 0;
    if (f->count) {
        const PlayerOrder *order = f->orders;
        switch (order->kind) {
        case 0x04:
            if (!walk(f, motion, &root, order, yaw, position, &done)) return 0;
            break;
        case 0x0a: case 0x07: {
            unsigned goal = order->kind == 0x07 ? (unsigned)order->x :
                (unsigned)bfm_waypoint_heading(root.xyz[0] - order->x, root.xyz[2] - order->z);
            done = bfm_twinpeak_turn(&root, goal);
            break;
        }
        case 0x19:
            if (!bfm_village_motion_box_step(motion, 0, 0, yaw, !f->entered, !f->entered, run))
                return 0;
            handled_motion = 1;
            done = f->entered && !bfm_village_motion_box_air_action(motion);
            break;
        case 0x1d:
            if (!bfm_woods_rootrick_notice_start(&f->notice, f->scene, f->scene_bytes, root.xyz))
                return 0;
            done = 1;
            break;
        case 0x46:
            f->talking = 1;
            f->talk_key = 0;
            f->talk_fusion = player->pose.show_weapon;
            done = 1;
            break;
        case 0x47:
            /* Retain the direct body through the following scripted turn. */
            done = 1;
            break;
        default: return 0;
        }
    }
    if (!handled_motion) {
        if (f->talking) {
            uint32_t packet = u32le(f->talk_packets + 4*f->talk_key) + 16;
            if (!bfm_village_motion_staged_model(motion, &root, packet, 0x13, f->talk_fusion, 0))
                return 0;
            f->talk_key = (f->talk_key + 1) % f->talk_count;
        } else if (!bfm_village_motion_staged(motion, &root)) return 0;
        for (unsigned i = 0; i < 3; ++i)
            position[i] = (int32_t)root.xyz[i] * 65536 + (uint16_t)position[i];
        if (!bfm_village_motion_staged_position(motion, position)) return 0;
    }
    if (f->count) {
        f->entered = !done;
        if (done) {
            --f->count;
            memmove(f->orders, f->orders + 1, f->count * sizeof f->orders[0]);
        }
    }
    return 1;
}

int bfm_earth_shrine_field_finish(BfmEarthShrineField *f, BfmVillageMotion *motion) {
    if (!f || !motion || f->count || f->window || f->shot.phase) return 0;
    if (f->started) {
        const BfmVillagePlayer *p = bfm_village_motion_player(motion);
        int32_t position[3];
        if (!p || !bfm_village_motion_position_q16(motion, position) ||
            !bfm_village_motion_place(motion, &p->root) ||
            !bfm_village_motion_staged_position(motion, position)) return 0;
    }
    if (f->shake_view) bfm_woods_box_view_shake(f->shake_view, 0);
    f->shake = (BfmWoodsRootrickImpact){0};
    f->shake_view = NULL;
    f->started = f->talking = 0;
    f->child = NULL;
    return 1;
}

static int half_toward(int from, int to) {
    int distance = to - from;
    return (distance == 1 || distance == -1) ? to : from + distance / 2;
}

void bfm_earth_shrine_field_camera(BfmEarthShrineField *f, BfmWoodsBoxView *view) {
    if (!f || !view) return;
    const BfmEarthShrineRelease *s = f->child;
    if (s && s->stage >= BFM_EARTH_RISING && s->stage <= BFM_EARTH_TAKEN && s->scale[0] > 0) {
        memcpy(f->primary_xyz, s->xyz, sizeof f->primary_xyz);
        f->primary_scale_y = s->scale[1];
        f->primary_sampled = 1;
    }
    if (f->shake.shake) {
        bfm_woods_rootrick_impact_tick(&f->shake);
        int offset = bfm_woods_rootrick_impact_camera_y(&f->shake);
        bfm_woods_box_view_shake(view, offset);
        f->shake_view = view;
        if (!f->shake.shaking && !offset && !f->shake.next_camera_y) {
            f->shake.shake = NULL;
            f->shake_view = NULL;
        }
    }
    if (!f->shot.phase && !f->tail_fields) return;
    const BfmVillageCamera *ordinary = bfm_woods_box_view_camera(view);
    if (!ordinary) return;
    BfmVillageCamera result = *ordinary;
    if (f->shot.phase) {
        /* This room keeps the final /6 update before blending to follow. */
        int last_return = f->returning && f->shot.return_ticks == 24;
        if (last_return) --f->shot.return_ticks;
        bfm_village_talk_camera_step(&f->shot, &result);
        if (last_return) {
            f->tail = result;
            f->tail_fields = 48;
            f->shot = (BfmVillageTalkCamera){0};
            f->returning = 0;
        }
    }
    if (f->tail_fields) {
        for (unsigned i = 0; i < 3; ++i) {
            result.eye[i] = f->tail.eye[i] = half_toward(f->tail.eye[i], ordinary->eye[i]);
            result.target[i] = f->tail.target[i] = half_toward(f->tail.target[i], ordinary->target[i]);
        }
        --f->tail_fields;
    }
    f->presented = result;
    bfm_woods_box_view_present(view, &result);
}

int bfm_earth_shrine_field_draw_notice(const BfmEarthShrineField *f, BfmBedroomFigure *figure,
                                       const BfmBedroomView *view) {
    return f && bfm_woods_rootrick_notice_add(&f->notice, figure, view);
}

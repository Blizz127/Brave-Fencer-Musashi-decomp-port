/* SPDX-License-Identifier: MIT */
#include "tower_summit_pickup.h"
#include "forest_dust.h"
#include "forest_opening_data.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const uint8_t *next, *end;
    unsigned interval, remaining;
    int held;
} BfmTowerPickupChannel;

typedef struct {
    BfmTowerPickupChannel body, face;
    const uint8_t *packed_pose;
    uint32_t visible, face_word;
    int8_t fusion_anchors[2][3], lumina_anchors[2][3];
    unsigned events;
    int complete, allow_turn_hold;
} BfmTowerPickupMotion;

enum {
    PICKUP_MOTION_LIFT, PICKUP_MOTION_SHEATHE
};
enum {
    PICKUP_MOTION_FACE_ADVANCE = 1u << 0,
    PICKUP_MOTION_EFFECT = 1u << 1,
    PICKUP_MOTION_SOUND_1225 = 1u << 2,
    PICKUP_MOTION_SOUND_1224 = 1u << 3
};

static int pickup_motion_word(BfmTowerPickupChannel *channel, uint32_t *word)
{
    if (channel->next == channel->end) return 0;
    *word = u32le(channel->next);
    channel->next += 4;
    return 1;
}

static int pickup_motion_advance(BfmTowerPickupMotion *m, const BfmForestField *field,
                                 BfmTowerPickupChannel *channel, int body)
{
    if (!channel->remaining || --channel->remaining) return 1;
    uint32_t command, value;
    while (pickup_motion_word(channel, &command)) {
        switch (command) {
        case 0: /* Hold the last display until the room selects a new track. */
            if (body) m->complete = 1;
            return 1;
        case 2: /* The lift holds its first pose until the turn completes. */
            if (body) {
                if (!m->allow_turn_hold) return 0;
                m->allow_turn_hold = 0;
            }
            channel->held = 1;
            return 1;
        case 3:
            if (!pickup_motion_word(channel, &value) || !value || value > 255) return 0;
            channel->interval = value;
            break;
        case 4:
            if (!body || !pickup_motion_word(channel, &value) ||
                value != UINT32_C(0x800de2a4)) return 0;
            break;
        case 7:
        case 8:
        case 9:
        case 10:
            if (!body) return 0;
            m->events |= 1u << (command - 7);
            break;
        case 17: /* Sheathe Fusion without changing Lumina. */
            if (!body) return 0;
            m->visible = (m->visible | (UINT32_C(1) << 21)) & ~(UINT32_C(1) << 22);
            break;
        case 18: /* Draw the newly equipped Lumina. */
            if (!body) return 0;
            m->visible = (m->visible | (UINT32_C(1) << 25)) & ~(UINT32_C(1) << 24);
            break;
        default:
            if (body) {
                if (command & 3u) return 0;
                const uint8_t *pose = bfm_forest_room_bytes(field, command, 256);
                if (!pose) return 0;
                m->packed_pose = pose;
            } else {
                /* Tagged values select resident VRAM faces; aligned MAIN resources
                 * request a facial texture upload. The texture owner handles them. */
                if ((command & ~7u) != UINT32_C(0x40000000) &&
                    (command < UINT32_C(0x800cedf8) || command >= UINT32_C(0x80128158) ||
                     (command & 3u))) return 0;
                m->face_word = command;
            }
            channel->remaining = channel->interval;
            return 1;
        }
    }
    return 0;
}

static int pickup_motion_display(BfmTowerPickupMotion *m, BfmForestPose *pose)
{
    const uint8_t *p = m->packed_pose;
    if (!p || !bfm_forest_pose_decode(p + 16, 240, pose)) return 0;
    memcpy(m->fusion_anchors, p, sizeof m->fusion_anchors);
    memcpy(m->lumina_anchors, p + 8, sizeof m->lumina_anchors);
    return 1;
}

/* Start once when the controller selects this motion. Its first body display
 * is loaded immediately; both channels then receive this update's service. */
static int pickup_motion_begin(BfmTowerPickupMotion *m, const BfmForestField *field,
                               unsigned clip, uint32_t parts, BfmForestPose *pose)
{
    static const struct {
        uint32_t body, face;
        unsigned body_words, face_words;
    } tracks[] = {
        {UINT32_C(0x8018eae8), UINT32_C(0x8018ecf0), 83, 8},
        {UINT32_C(0x8018ec34), UINT32_C(0x8018ed10), 47, 4}
    };
    if (!m || !field || !pose || clip >= sizeof tracks / sizeof tracks[0]) return 0;
    size_t body_size = 4 * tracks[clip].body_words, face_size = 4 * tracks[clip].face_words;
    const uint8_t *body = bfm_forest_room_bytes(field, tracks[clip].body, body_size);
    const uint8_t *face = bfm_forest_room_bytes(field, tracks[clip].face, face_size);
    if (!body || !face) return 0;
    BfmTowerPickupMotion next = {.visible = parts, .allow_turn_hold = clip == PICKUP_MOTION_LIFT,
        .body = {body, body + body_size, 1, 1, 0},
        .face = {face, face + face_size, 1, 1, 0}};
    if (!pickup_motion_advance(&next, field, &next.body, 1) ||
        !pickup_motion_advance(&next, field, &next.body, 1) ||
        !pickup_motion_advance(&next, field, &next.face, 0) ||
        (clip == PICKUP_MOTION_LIFT && !next.body.held) ||
        !pickup_motion_display(&next, pose)) return 0;
    *m = next;
    return 1;
}

/* A face cue belongs to the room action that observes the previous body's
 * markers. Keep the held display on updates without a new pose word. */
static int pickup_motion_tick(BfmTowerPickupMotion *m, const BfmForestField *field,
                              int resume_body, int resume_face, BfmForestPose *pose)
{
    if (!m || !field || !pose || !m->packed_pose) return 0;
    BfmTowerPickupMotion next = *m;
    next.events = 0;
    if (resume_body && next.body.held) {
        next.body.held = 0;
        next.body.remaining = next.body.interval;
    }
    if (resume_face && next.face.held) {
        next.face.held = 0;
        next.face.remaining = next.face.interval;
    }
    if (!pickup_motion_advance(&next, field, &next.body, 1) ||
        !pickup_motion_advance(&next, field, &next.face, 0) ||
        !pickup_motion_display(&next, pose)) return 0;
    *m = next;
    return 1;
}

static void pickup_motion_sheathe(BfmTowerPickupMotion *m)
{
    m->visible = (m->visible | (UINT32_C(1) << 24)) & ~(UINT32_C(1) << 25);
}

typedef struct {
    int16_t distance, pitch, bearing_offset, focus_x, focus_y;
} BfmSummitPickupOrbit;

typedef struct {
    BfmSummitPickupOrbit orbit, previous;
    int orbiting;
} BfmSummitPickupCamera;

typedef enum {
    BFM_PICKUP_CAMERA_CLOSE,
    BFM_PICKUP_CAMERA_ORBIT,
    BFM_PICKUP_CAMERA_RESTORE
} BfmSummitPickupCameraCue;


static void bfm_summit_pickup_camera_start(BfmSummitPickupCamera *camera)
{
    if (camera)
        *camera = (BfmSummitPickupCamera){.orbit = {1000, 128, 512, 80, -200}};
}

/* The lens and offsets ease independently. Small residuals use a shorter
 * divisor so each nonzero gap still advances at least one world unit. */
static int16_t pickup_camera_approach(int from, int to, int divisor)
{
    int delta = to - from;
    while (divisor > 1 && delta / divisor == 0)
        divisor /= 2;
    return (int16_t)(from + delta / divisor);
}

static void bfm_summit_pickup_camera_tick(BfmSummitPickupCamera *camera,
                                   BfmSummitPickupCameraCue cue)
{
    if (!camera) return;
    BfmSummitPickupOrbit *orbit = &camera->orbit;
    if (cue == BFM_PICKUP_CAMERA_ORBIT) {
        if (!camera->orbiting) {
            camera->previous = *orbit;
            camera->orbiting = 1;
            orbit->distance = 256;
            orbit->pitch = 113;
            orbit->focus_y = 64;
        }
        orbit->bearing_offset = (int16_t)((orbit->bearing_offset + 22) & 4095);
        --orbit->focus_y;
        return;
    }
    if (camera->orbiting) {
        if (cue != BFM_PICKUP_CAMERA_RESTORE) return;
        *orbit = camera->previous;
        camera->orbiting = 0;
    }
    orbit->distance = pickup_camera_approach(orbit->distance, 300, 4);
    orbit->pitch = pickup_camera_approach(orbit->pitch, -128, 4);
    orbit->bearing_offset = pickup_camera_approach(orbit->bearing_offset, 0, 4);
    orbit->focus_x = pickup_camera_approach(orbit->focus_x, 0, 16);
    orbit->focus_y = pickup_camera_approach(orbit->focus_y, -80, 16);
}

static int bfm_summit_pickup_camera_view(const BfmSummitPickupCamera *camera,
                                  const int16_t player[3],
                                  BfmForestCamera *view, int32_t eye[3])
{
    if (!camera || !player || !view || !eye || !camera->orbit.distance) return 0;
    const BfmSummitPickupOrbit *orbit = &camera->orbit;
    int32_t target[3] = {player[0] + orbit->focus_x,
                         player[1] + orbit->focus_y, player[2]};
    unsigned yaw = bfm_forest_shore_bearing(player[0], player[2]) + orbit->bearing_offset;
    BfmForestTransform arm;
    bfm_forest_actor_pitch_yaw((unsigned)orbit->pitch & 4095, yaw & 4095, player, &arm);
    for (unsigned i = 0; i < 3; ++i) {
        int scaled = orbit->distance * arm.rotation[i][2];
        int offset = scaled / 4096 - (scaled % 4096 < 0);
        eye[i] = target[i] + offset;
        if (orbit->distance > 724)
            target[i] = eye[i] - offset / 2;
    }
    return bfm_forest_camera_look_at(eye, target, 300, view);
}

static void lumina_glow_quad(const BfmForestCamera *local, const int16_t xyz[4][3],
                             uint32_t color, int both_centers,
                             BfmForestDustCard *out, size_t *count)
{
    BfmForestDustCard card = {0};
    if (!bfm_forest_project_effect_quad(local, xyz, &card)) return;
    card.material = (BfmForestRasterMaterial){.page = 32, .count = 4, .translucent = 1};
    card.vertices[1].rgb = color;
    if (both_centers) card.vertices[0].rgb = color;
    out[(*count)++] = card;
}

/* The mounted outline supplies four sectors around the hilt, two around
 * each end of the blade, and the two sides joining the blade's end caps. */
size_t bfm_tower_summit_pickup_glow(const BfmForestField *room,
                                   const int8_t lumina_anchors[2][3],
                                   const BfmForestTransform *root, const int32_t eye[3],
                                   const BfmForestCamera *camera, unsigned scene_tick,
                                   BfmForestDustCard *out, size_t capacity)
{
    if (!room || !lumina_anchors || !root || !eye || !camera || !out || capacity < 10) return 0;
    const int8_t *rim = (const int8_t *)bfm_forest_room_bytes(room, UINT32_C(0x8018dce4), 48);
    BfmForestTransform halo;
    BfmForestCamera local;
    if (!rim || !bfm_forest_blade_transform(lumina_anchors, root, eye, &halo) ||
        !bfm_forest_effect_camera(camera, &halo, &local)) return 0;

    unsigned red = (scene_tick & 1u) ? 160 : (scene_tick & 2u) ? 192 : 128;
    uint32_t color = red | (red / 4u << 8) | (red / 4u << 16);
    size_t count = 0;
    for (unsigned arc = 0; arc < 4; ++arc) {
        const int8_t *p = rim + arc * 4;
        const int16_t xyz[4][3] = {{p[0], p[1], 0}, {0, 0, 0},
                                    {p[2], p[3], 0}, {p[4], p[5], 0}};
        lumina_glow_quad(&local, xyz, color, 0, out, &count);
    }

    const int8_t *blade = rim + 20;
    for (unsigned end = 0; end < 2; ++end) {
        int16_t center = end ? 32 : -32;
        for (unsigned arc = 0; arc < 2; ++arc) {
            const int8_t *p = blade + 10 * end + 4 * arc;
            const int16_t xyz[4][3] = {{p[0], 0, p[1]}, {0, 0, center},
                                        {p[2], 0, p[3]}, {p[4], 0, p[5]}};
            lumina_glow_quad(&local, xyz, color, 0, out, &count);
        }
    }
    for (unsigned side = 0; side < 2; ++side) {
        const int8_t *p = blade + 20 + 4 * side;
        const int16_t xyz[4][3] = {{0, 0, -32}, {0, 0, 32},
                                    {p[0], 0, p[1]}, {p[2], 0, p[3]}};
        lumina_glow_quad(&local, xyz, color, 1, out, &count);
    }
    return count;
}

enum PickupPhase { PREPARE, POSITION, FACE, LIFT, SHEATHE, READ_BANK, FOLLOWUP };

struct BfmTowerSummitPickup {
    const BfmForestField *room;
    const MusashiDiscMedia *disc;
    int16_t position[3];
    enum PickupPhase phase;
    BfmTowerPickupMotion motion;
    BfmSummitPickupCamera camera;
    BfmSummitPickupFrame frame;
    unsigned orbit_delay, read_updates;
    BfmPalaceBank *bank;
};

BfmTowerSummitPickup *bfm_tower_summit_pickup_begin(const BfmForestField *room,
    const MusashiDiscMedia *disc, uint32_t visible_parts)
{
    if (!room || !disc) return NULL;
    const uint8_t *entrance = bfm_forest_field_entrance(room, 0);
    const uint8_t *point = bfm_forest_room_bytes(room, UINT32_C(0x801e6c7c), 8);
    if (!entrance || u16le(entrance) != 0x300b || u16le(entrance + 2) || !point)
        return NULL;
    BfmTowerSummitPickup *p = calloc(1, sizeof *p);
    if (!p) return NULL;
    p->room = room;
    p->disc = disc;
    p->frame.visible_parts = visible_parts;
    for (unsigned i = 0; i < 3; ++i) p->position[i] = s16le(point + 2*i);
    bfm_summit_pickup_camera_start(&p->camera);
    return p;
}

void bfm_tower_summit_pickup_free(BfmTowerSummitPickup *p)
{
    if (p) {
        bfm_palace_sound_bank_free(p->bank);
        free(p);
    }
}

BfmSummitPickupGoal bfm_tower_summit_pickup_goal(const BfmTowerSummitPickup *p,
    int16_t target[3], int16_t *yaw)
{
    if (!p) return BFM_SUMMIT_PICKUP_HOLD;
    if (target) memcpy(target, p->position, sizeof p->position);
    if (yaw) *yaw = 3072;
    switch (p->phase) {
    case POSITION: return BFM_SUMMIT_PICKUP_ALIGN;
    case FACE: return BFM_SUMMIT_PICKUP_TURN;
    case FOLLOWUP: return BFM_SUMMIT_PICKUP_FOLLOWUP;
    default: return BFM_SUMMIT_PICKUP_HOLD;
    }
}

static int pickup_read_bank(BfmTowerSummitPickup *p)
{
    if (!p->bank) {
        /* Warm optical request, seek, then five sectors per 30 Hz update. */
        enum { sector_bytes = 2048, sectors_per_update = 5, warm_start = 2 + 13 };
        uint8_t directory[32];
        size_t file_bytes;
        if (!musashi_disc_media_read_root_range(p->disc, "SC02.CD", 0, directory,
                sizeof directory, &file_bytes) || u32le(directory) <= 2) return 0;
        uint32_t length = u32le(directory + 28);
        if (!length || length > file_bytes) return 0;
        p->bank = bfm_archive_sound_bank_load(p->disc, "SC02.CD", 2);
        if (!p->bank) return 0;
        uint32_t sectors = 1u + (length - 1u) / sector_bytes;
        p->read_updates = warm_start + (sectors + sectors_per_update - 1) / sectors_per_update;
    }
    if (--p->read_updates == 0) p->phase = FOLLOWUP;
    return 1;
}

static void pickup_frame(BfmTowerSummitPickup *p)
{
    p->frame.visible_parts = p->motion.visible;
    p->frame.face_resource = p->motion.face_word;
    memcpy(p->frame.fusion_anchors, p->motion.fusion_anchors, sizeof p->frame.fusion_anchors);
    memcpy(p->frame.lumina_anchors, p->motion.lumina_anchors, sizeof p->frame.lumina_anchors);
    p->frame.has_pose = 1;
}

int bfm_tower_summit_pickup_tick(BfmTowerSummitPickup *p, BfmGameplayRun *run,
    int aligned, int facing, BfmForestPose *pose)
{
    if (!p || !run || !run->has_player || !pose) return 0;
    p->frame.sound_command = p->frame.score_selector = 0;
    if (p->phase == FOLLOWUP) return pickup_motion_display(&p->motion, pose);
    BfmSummitPickupCameraCue camera = BFM_PICKUP_CAMERA_CLOSE;
    unsigned events = p->motion.events;
    switch (p->phase) {
    case PREPARE:
        p->phase = POSITION;
        break;
    case POSITION:
        if (aligned) {
            if (!pickup_motion_begin(&p->motion, p->room, PICKUP_MOTION_LIFT,
                                     p->frame.visible_parts, pose)) return 0;
            if (!bfm_gameplay_acquire_lumina(run)) return 0;
            p->phase = FACE;
        }
        break;
    case FACE:
        if (!pickup_motion_tick(&p->motion, p->room, facing, 0, pose)) return 0;
        if (facing) p->phase = LIFT;
        break;
    case LIFT:
        if (p->motion.complete) {
            if (!pickup_motion_begin(&p->motion, p->room, PICKUP_MOTION_SHEATHE,
                                     p->motion.visible, pose)) return 0;
            p->phase = SHEATHE;
            camera = BFM_PICKUP_CAMERA_RESTORE;
            break;
        }
        if (p->orbit_delay && --p->orbit_delay == 0) {
            camera = BFM_PICKUP_CAMERA_ORBIT;
            p->frame.score_selector = 19;
        } else if (p->camera.orbiting) {
            camera = BFM_PICKUP_CAMERA_ORBIT;
        }
        if (events & PICKUP_MOTION_EFFECT) p->orbit_delay = 2;
        if (events & PICKUP_MOTION_SOUND_1224) p->frame.sound_command = 1224;
        if (events & PICKUP_MOTION_SOUND_1225) p->frame.sound_command = 1225;
        if (!pickup_motion_tick(&p->motion, p->room, 0, events & PICKUP_MOTION_FACE_ADVANCE,
                                pose)) return 0;
        break;
    case SHEATHE:
        if (p->motion.complete) {
            if (!pickup_motion_display(&p->motion, pose)) return 0;
            p->phase = READ_BANK;
            break;
        }
        if (events & PICKUP_MOTION_EFFECT) {
            pickup_motion_sheathe(&p->motion);
            p->frame.sound_command = 1093;
        }
        if (!pickup_motion_tick(&p->motion, p->room, 0, events & PICKUP_MOTION_FACE_ADVANCE,
                                pose)) return 0;
        break;
    case READ_BANK:
        if (!pickup_motion_display(&p->motion, pose) || !pickup_read_bank(p)) return 0;
        break;
    case FOLLOWUP:
        break;
    }
    if (p->motion.packed_pose) pickup_frame(p);
    bfm_summit_pickup_camera_tick(&p->camera, camera);
    return 1;
}

const BfmSummitPickupFrame *bfm_tower_summit_pickup_frame(const BfmTowerSummitPickup *p)
{
    return p ? &p->frame : NULL;
}

int bfm_tower_summit_pickup_camera(const BfmTowerSummitPickup *p,
    const int16_t player[3], BfmForestCamera *camera, int32_t eye[3])
{
    return p && bfm_summit_pickup_camera_view(&p->camera, player, camera, eye);
}

BfmPalaceBank *bfm_tower_summit_pickup_take_bank(BfmTowerSummitPickup *p)
{
    if (!p || p->phase != FOLLOWUP) return NULL;
    BfmPalaceBank *bank = p->bank;
    p->bank = NULL;
    return bank;
}

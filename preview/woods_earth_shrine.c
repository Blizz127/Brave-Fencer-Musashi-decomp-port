/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine.h"
#include "field_waypoint.h"
#include "woods_twinpeak_stride.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum ShrinePhase { ABSENT, WAITING, WALKING, FACING, REQUEST_SCORE, SPEAKING, RELEASE, EXAMINED };
enum { SCENE_ORIGIN = 0x80128158u, ROOM = 0x801961e8u, DISCOVERY_SCORE = 0x2f };

struct BfmEarthShrine {
    const uint8_t *speech;
    size_t speech_bytes;
    int16_t position[3], limits[3][2], walk_x, walk_z;
    enum ShrinePhase phase;
    int walking;
    unsigned delay;
    int score_ready;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (address < SCENE_ORIGIN) return NULL;
    size_t offset = address - SCENE_ORIGIN;
    return offset <= bytes && length <= bytes-offset ? scene+offset : NULL;
}

BfmEarthShrine *bfm_earth_shrine_open(const uint8_t *scene, size_t bytes, int earned)
{
    if (!scene) return NULL;
    const uint8_t *room = mounted(scene, bytes, ROOM, 0xbc);
    const uint8_t *volume = mounted(scene, bytes, 0x801958f4u, 12);
    const uint8_t *program = mounted(scene, bytes, 0x80195910u, 56);
    if (!room || !volume || !program) return NULL;
    uint32_t first = u32le(room+0xb4), end = u32le(room+0xb8);
    if (end <= first || (end-first)%20) return NULL;
    const uint8_t *actors = mounted(scene, bytes, first, end-first);
    if (!actors) return NULL;
    const uint8_t *altar = NULL;
    for (size_t i=0; i < end-first; i+=20) {
        const uint8_t *row = actors+i;
        if (!u16le(row+6)) break;
        if (u16le(row+6)==268 && !u16le(row+8)) {
            if (altar) return NULL;
            altar = row;
        }
    }
    if (!altar || u32le(program)!=3 || u32le(program+8)!=0x40 ||
        u32le(program+16)!=0x0a || u32le(program+24)!=0x3e ||
        u32le(program+28)!=0x80185bf4u || u32le(program+32)!=0x3e ||
        u32le(program+36)!=0x80185c00u || u32le(program+40)!=1 ||
        u32le(program+48)!=2) return NULL;
    uint32_t speech = u32le(program+44);
    /* The award notice immediately follows the discovery line in this room. */
    const uint32_t award_notice = UINT32_C(0x80196960);
    if (speech >= award_notice) return NULL;
    size_t speech_bytes = award_notice - speech;
    const uint8_t *text = mounted(scene, bytes, speech, speech_bytes);
    if (!text || *text != 5) return NULL;
    BfmEarthShrine *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->phase = earned ? ABSENT : WAITING;
    s->speech = text;
    s->speech_bytes = speech_bytes;
    for (unsigned i=0; i<3; i++) {
        s->position[i] = s16le(altar+2*i);
        s->limits[i][0] = s16le(volume+4*i);
        s->limits[i][1] = s16le(volume+4*i+2);
        if (s->limits[i][0] >= s->limits[i][1]) {
            free(s);
            return NULL;
        }
    }
    s->walk_x = s16le(program+4);
    s->walk_z = s16le(program+6);
    return s;
}

void bfm_earth_shrine_free(BfmEarthShrine *s)
{
    free(s);
}

int bfm_earth_shrine_on_approach(BfmEarthShrine *s, const BfmBedroomRoot *player)
{
    if (s->phase != WAITING) return 1;
    for (unsigned i=0; i<3; i++) {
        int delta = player->xyz[i] - s->position[i];
        if (delta <= s->limits[i][0] || delta >= s->limits[i][1]) return 1;
    }
    s->delay = 32;
    s->phase = WALKING;
    return 1;
}

int bfm_earth_shrine_in_scene(const BfmEarthShrine *s)
{
    return s->phase >= WALKING && s->phase <= RELEASE;
}

/* The lip beside the tablet has an oblique wall over a level floor.
 * Project against both faces in authored order before reading the normal
 * of the support below the corrected foot. */
static int step_to_tablet(const BfmVillageGround *ground, BfmBedroomRoot *root, int16_t to[3])
{
    uint16_t material;
    if (!(bfm_village_ground_support_probe(ground, 0, root->xyz, to, &material) & 0x6000))
        return 0;
    int16_t from[3];
    memcpy(from, to, sizeof from);
    BfmForestContact floor;
    if (bfm_village_ground_step(ground, from, to, &floor) != BFM_VILLAGE_GROUND_FOOTING)
        return 0;
    memcpy(root->xyz, to, sizeof root->xyz);
    memcpy(root->normal, floor.normal, sizeof root->normal);
    return 1;
}

int bfm_earth_shrine_update(BfmEarthShrine *s, BfmVillageMotion *motion, BfmWoodsBoxView *view,
                            BfmForestMessage **opened)
{
    const BfmVillageCamera *camera = bfm_woods_box_view_camera(view);
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!player || !camera || !opened || *opened) return 0;
    BfmBedroomRoot root = player->root;
    int32_t position[3];
    if (!bfm_village_motion_position_q16(motion, position)) return 0;
    if (s->phase == RELEASE) {
        if (!bfm_village_motion_release_staged(motion)) return 0;
        s->phase = EXAMINED;
        return 1;
    }
    if (s->delay) --s->delay;
    root.locomotion_edge = BFM_TRAVEL_NONE;
    int facing_tablet = 0;
    if (s->phase == WALKING) {
        int dx = s->walk_x - root.xyz[0], dz = s->walk_z - root.xyz[2];
        const BfmVillageGround *ground = bfm_village_motion_ground(motion);
        int16_t to[3] = {s->walk_x, root.xyz[1], s->walk_z};
        if ((int64_t)dx * dx + (int64_t)dz * dz < 16 * 16) {
            if (!step_to_tablet(ground, &root, to)) return 0;
            root.locomotion_edge = s->walking ? BFM_TRAVEL_STAND : BFM_TRAVEL_NONE;
            s->phase = FACING;
        } else {
            int yaw = bfm_village_motion_view_yaw(camera->eye, camera->target);
            BfmWaypointInput stick = bfm_waypoint_input(dx, dz, 0, yaw);
            root.yaw = (unsigned)stick.yaw;
            if (!bfm_twinpeak_stride_target(&root, position, stick.speed_q16, root.yaw, to) ||
                !step_to_tablet(ground, &root, to)) return 0;
            if (!s->walking) root.locomotion_edge = BFM_TRAVEL_WALK;
            s->walking = 1;
        }
    }
    if (s->phase != WALKING)
        facing_tablet = bfm_twinpeak_turn(&root, bfm_waypoint_heading(root.xyz[0] - s->position[0],
                                                      root.xyz[2] - s->position[2]));
    for (unsigned i = 0; i < 3; ++i)
        position[i] = (int32_t)root.xyz[i] * 65536 + (uint16_t)position[i];
    if (!bfm_village_motion_staged(motion, &root) ||
        !bfm_village_motion_staged_position(motion, position)) return 0;
    bfm_woods_box_view_follow(view, root.xyz, 0, 0);
    if (s->phase == FACING && facing_tablet && !s->delay) s->phase = REQUEST_SCORE;
    if (s->phase == REQUEST_SCORE && s->score_ready) {
        *opened = bfm_field_message_borrow(s->speech, s->speech_bytes);
        if (!*opened) return 0;
        s->phase = SPEAKING;
    }
    return 1;
}

int bfm_earth_shrine_closed(BfmEarthShrine *s)
{
    if (s->phase != SPEAKING) return 0;
    s->phase = RELEASE;
    return 1;
}

unsigned bfm_earth_shrine_score(const BfmEarthShrine *s, unsigned field_score)
{
    if (s->phase < WALKING) return field_score;
    return s->phase < REQUEST_SCORE ? 0 : DISCOVERY_SCORE;
}

void bfm_earth_shrine_score_applied(BfmEarthShrine *s, unsigned selector)
{
    if (s->phase == REQUEST_SCORE && selector == DISCOVERY_SCORE) s->score_ready = 1;
}

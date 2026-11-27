/* SPDX-License-Identifier: MIT */
#include "village_jon_stage.h"
#include "forest_pose.h"
#include "village_talk_camera.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>

enum { IDLE, ACCEPTED, WAIT_MEMBER, FRAMED, RETURNING };
static const uint32_t scene_origin = UINT32_C(0x80128158);
static const uint32_t event_origin = UINT32_C(0x801ef468);

struct BfmVillageJonStage {
    const uint8_t *scene, *station, *framing;
    size_t bytes;
    uint32_t room;
    int16_t destination[2];
    BfmBedroomTravel *travel;
    BfmVillageTalkCamera shot;
    unsigned phase;
    int facing_jon;
    BfmBedroomAction queued_reply, next_reply, action;
};

static const uint8_t *record(const uint8_t *data, size_t bytes, uint32_t origin,
                             uint32_t address, size_t count)
{
    if (address < origin) return NULL;
    size_t offset = address - origin;
    return offset <= bytes && count <= bytes - offset ? data + offset : NULL;
}

static int floor_q12(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

static int destination(const BfmVillageJonStage *s, int x, int z, int16_t goal[2])
{
    /* The walk destination is local to Jon's actual model root. The queued
     * facing command uses Jon himself, independently of the camera bearing. */
    int16_t xyz[3];
    for (unsigned i = 0; i < 3; ++i) xyz[i] = s16le(s->station + 2*i);
    BfmForestTransform root;
    bfm_forest_actor_root(u16le(s->station + 6) & 4095u, xyz, &root);
    for (unsigned i = 0; i < 2; ++i) {
        unsigned axis = 2*i;
        int world = xyz[axis] + floor_q12((int64_t)root.rotation[axis][0]*x +
                                          (int64_t)root.rotation[axis][2]*z);
        if (world < INT16_MIN || world > INT16_MAX) return 0;
        goal[i] = (int16_t)world;
    }
    return 1;
}

static int read_approach(BfmVillageJonStage *s, const uint8_t *module, size_t bytes)
{
    const uint8_t *first = record(module, bytes, event_origin, UINT32_C(0x801f10c0), 16);
    if (!first || u32le(first + 8) != 0x35) return 0;
    const uint8_t *cue = record(module, bytes, event_origin, u32le(first + 12), 32);
    if (!cue || u32le(cue + 8) != 0x21 || u32le(cue + 16) != 4 || u32le(cue + 24) != 10)
        return 0;
    s->framing = record(module, bytes, event_origin, u32le(cue + 12), 16);
    return s->framing && s16le(s->framing + 10) > 0 &&
           destination(s, s16le(cue + 20), s16le(cue + 22), s->destination);
}

BfmVillageJonStage *bfm_village_jon_stage_open(const uint8_t *scene, size_t bytes,
                    uint32_t room, const uint8_t *module, size_t module_bytes)
{
    if (!scene || !module || room != UINT32_C(0x80192b18)) return NULL;
    BfmVillageJonStage *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->scene = scene;
    s->bytes = bytes;
    s->room = room;
    s->station = record(scene, bytes, scene_origin, UINT32_C(0x80192a64), 8);
    if (!s->station || !read_approach(s, module, module_bytes)) {
        free(s);
        return NULL;
    }
    return s;
}

void bfm_village_jon_stage_free(BfmVillageJonStage *s)
{
    if (!s) return;
    bfm_bedroom_travel_free(s->travel);
    free(s);
}

int bfm_village_jon_stage_begin(BfmVillageJonStage *s, const BfmBedroomRoot *player,
                               int square_pressed)
{
    if (!s || !player || !square_pressed || s->phase != IDLE) return 0;
    BfmBedroomTravel *travel = bfm_bedroom_travel_from_root(s->scene, s->bytes, s->room, player);
    if (!travel) return 0;
    bfm_bedroom_travel_free(s->travel);
    s->travel = travel;
    s->facing_jon = 0;
    s->queued_reply = s->next_reply = s->action = BFM_BEDROOM_NONE;
    s->phase = ACCEPTED;
    return 1;
}

BfmVillageJonStageEvent bfm_village_jon_stage_step(BfmVillageJonStage *s, int member_ready,
                                                   BfmVillageCamera *presented)
{
    if (!s || !presented) return BFM_JON_STAGE_ERROR;
    s->action = s->next_reply;
    s->next_reply = BFM_BEDROOM_NONE;
    if (s->phase == IDLE) return BFM_JON_STAGE_WAIT;
    if (s->phase == ACCEPTED) {
        s->phase = WAIT_MEMBER;
        return BFM_JON_STAGE_WAIT;
    }
    if (s->phase == WAIT_MEMBER) {
        if (!member_ready) return BFM_JON_STAGE_WAIT;
        if (!bfm_village_talk_camera_frame(&s->shot, s->station, s->framing, presented) ||
            !bfm_bedroom_travel_walk(s->travel, s->destination[0], s->destination[1], 0))
            return BFM_JON_STAGE_ERROR;
        s->phase = FRAMED;
        return BFM_JON_STAGE_INSTALL;
    }

    if (!bfm_bedroom_travel_step(s->travel, s->shot.eye, s->shot.target))
        return BFM_JON_STAGE_ERROR;
    const BfmBedroomRoot *player = bfm_bedroom_travel_root(s->travel);
    if (!s->facing_jon && !player->busy && player->locomotion_edge == BFM_TRAVEL_NONE)
        s->facing_jon = bfm_bedroom_travel_turn_to_point(s->travel,
                                            s16le(s->station), s16le(s->station + 4));
    /* The field queue admits one actor job after Musashi's update. His
     * next update selects its body program while the open line continues. */
    if (s->queued_reply != BFM_BEDROOM_NONE && s->facing_jon && !player->busy) {
        s->next_reply = s->queued_reply;
        s->queued_reply = BFM_BEDROOM_NONE;
    }
    bfm_village_talk_camera_step(&s->shot, presented);
    if (s->phase == RETURNING && !s->shot.phase) {
        s->phase = IDLE;
        return BFM_JON_STAGE_RESTORED;
    }
    return BFM_JON_STAGE_WAIT;
}

int bfm_village_jon_stage_walk(BfmVillageJonStage *s, int local_x, int local_z)
{
    int16_t goal[2];
    if (!s || s->phase != FRAMED ||
        local_x < INT16_MIN || local_x > INT16_MAX || local_z < INT16_MIN || local_z > INT16_MAX ||
        !destination(s, local_x, local_z, goal) ||
        !bfm_bedroom_travel_walk(s->travel, goal[0], goal[1], 0)) return 0;
    s->facing_jon = 0;
    return 1;
}

int bfm_village_jon_stage_react(BfmVillageJonStage *s, int affirmative)
{
    if (!s || s->phase != FRAMED || s->queued_reply != BFM_BEDROOM_NONE) return 0;
    s->queued_reply = affirmative ? BFM_BEDROOM_FAREWELL : BFM_BEDROOM_NEGATIVE_REPLY;
    return 1;
}

BfmBedroomAction bfm_village_jon_stage_action(const BfmVillageJonStage *s)
{
    return s ? s->action : BFM_BEDROOM_NONE;
}

void bfm_village_jon_stage_finish(BfmVillageJonStage *s)
{
    if (s && s->phase == FRAMED) {
        bfm_village_talk_camera_release(&s->shot);
        s->phase = RETURNING;
    }
}

int bfm_village_jon_stage_active(const BfmVillageJonStage *s)
{
    return s && s->phase != IDLE;
}

const BfmBedroomRoot *bfm_village_jon_stage_player(const BfmVillageJonStage *s)
{
    return s ? bfm_bedroom_travel_root(s->travel) : NULL;
}

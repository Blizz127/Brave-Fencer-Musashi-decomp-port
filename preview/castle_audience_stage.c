/* SPDX-License-Identifier: MIT */
#include "castle_audience_stage.h"
#include "bedroom_visitor.h"
#include "village_field.h"
#include <stdlib.h>

#define FIELD_BASE UINT32_C(0x80128158)
#define THRONE_ROOM UINT32_C(0x801863f8)

struct BfmCastleAudienceStage {
    const uint8_t *scene;
    size_t scene_bytes;
    uint16_t (*tiles)[1024];
    BfmVillageField field;
    BfmBedroomTravel *travel;
    BfmBedroomMotion *motion;
    BfmBedroomPose pose;
    BfmBedroomVisitor *steward;
    int home[3], heading;
    uint32_t breathing[10];
    unsigned age;
    unsigned approach_settle;
    int closeup, exiting;
};

static const int entrance_eye[3] = {0, -275, -885};
static const int entrance_target[3] = {0, -275, -185};

static const uint8_t *mounted(const BfmCastleAudienceStage *s, uint32_t address, size_t count)
{
    if (address < FIELD_BASE || count > s->scene_bytes) return NULL;
    size_t at = address - FIELD_BASE;
    return at <= s->scene_bytes - count ? s->scene + at : NULL;
}

static unsigned half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t word(const uint8_t *p)
{
    return half(p) | (uint32_t)half(p + 2) << 16;
}

void bfm_castle_audience_stage_free(BfmCastleAudienceStage *s)
{
    if (!s) return;
    bfm_bedroom_visitor_free(s->steward);
    bfm_bedroom_motion_free(s->motion);
    bfm_bedroom_travel_free(s->travel);
    bfm_village_field_clear(&s->field);
    free(s);
}

BfmCastleAudienceStage *bfm_castle_audience_stage_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes,
    uint16_t tiles[512][1024], unsigned equipped_orb)
{
    if (!disc || !scene || !main || !tiles) return NULL;
    BfmCastleAudienceStage *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->scene = scene;
    s->scene_bytes = scene_bytes;
    s->tiles = tiles;
    const uint8_t *room = mounted(s, THRONE_ROOM, 0xcc);
    const uint8_t *actor = room ? mounted(s, word(room + 0xb4), 20) : NULL;
    const uint8_t *duration = mounted(s, 0x801863d8, 4);
    if (!actor || half(actor + 6) != 365 || !duration) goto fail;
    for (unsigned i = 0; i < 3; ++i) s->home[i] = (int16_t)half(actor + 2*i);
    s->heading = half(actor + 12) & 4095;
    if (!bfm_village_field_load_room(&s->field, disc, scene, scene_bytes,
                                     THRONE_ROOM, 0x801847a4)) goto fail;
    s->travel = bfm_bedroom_travel_open(scene, scene_bytes, THRONE_ROOM, 0);
    s->motion = bfm_bedroom_motion_open(scene, scene_bytes, main, main_bytes, THRONE_ROOM);
    /* Ribson's ten one-field poses loop for the audience. */
    const uint8_t *clip = mounted(s, 0x801e9088, 11 * 8);
    if (!clip || word(clip + 80) != 0 || word(clip + 84) != 65535) goto fail;
    for (unsigned i = 0; i < 10; ++i) {
        uint32_t key = word(clip + 8*i);
        uint32_t pose = key & ~UINT32_C(0x01000000);
        if ((key >> 24) != 0x81 || word(clip + 8*i + 4) != 1 ||
            !mounted(s, pose, 7 * 8)) goto fail;
        s->breathing[i] = pose;
    }
    const uint32_t steward_model = 0x801aaedc;
    s->steward = bfm_bedroom_visitor_load(scene, scene_bytes, &steward_model, 1, 0x80184290);
    if (!s->travel || !s->motion || !s->steward ||
        !bfm_bedroom_orb_palette(s->motion, 0x80186140, equipped_orb, tiles) ||
        !bfm_bedroom_motion_step(s->motion, BFM_BEDROOM_ENTER, &s->pose) ||
        !bfm_bedroom_travel_entrance(s->travel, word(duration))) goto fail;
    bfm_bedroom_field_viewpoint(s->field.field, entrance_eye, entrance_target, 1, 1);
    return s;
fail:
    bfm_castle_audience_stage_free(s);
    return NULL;
}

int bfm_castle_audience_stage_speakers(const BfmCastleAudienceStage *s,
                                     int *musashi_x, int *ribson_x)
{
    if (!s || !musashi_x || !ribson_x) return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(s->field.field);
    const BfmBedroomRoot *hero = bfm_bedroom_travel_root(s->travel);
    int16_t steward[3];
    int player_screen[3], steward_screen[3];
    for (unsigned i = 0; i < 3; ++i) steward[i] = (int16_t)s->home[i];
    if (!bfm_bedroom_project(view, hero->xyz, player_screen) ||
        !bfm_bedroom_project(view, steward, steward_screen)) return 0;
    *musashi_x = player_screen[0] - 160;
    *ribson_x = steward_screen[0] - 160;
    return 1;
}

void bfm_castle_audience_stage_begin_exit(BfmCastleAudienceStage *s)
{
    if (!s || s->exiting) return;
    s->exiting = 1;
    bfm_bedroom_field_viewpoint(s->field.field, entrance_eye, entrance_target, 0, 6);
}

int bfm_castle_audience_stage_step(BfmCastleAudienceStage *s)
{
    if (!s) return 0;
    const BfmBedroomView *v = bfm_bedroom_field_view(s->field.field);
    if (!v || !bfm_bedroom_travel_step(s->travel, v->eye, v->target)) return 0;
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(s->travel);
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    if (root->locomotion_edge == BFM_TRAVEL_WALK) action = BFM_BEDROOM_WALK;
    if (root->locomotion_edge == BFM_TRAVEL_STAND) action = BFM_BEDROOM_STAND;
    if (!s->closeup && !s->exiting && !root->busy) {
        if (++s->approach_settle == 3) {
            const uint8_t *opening = mounted(s, 0x801e91a8, 8);
            if (!opening || word(opening) != 0x21 ||
                !bfm_bedroom_field_camera(s->field.field, s->scene, s->scene_bytes,
                                          word(opening + 4), s->home, 0)) return 0;
            s->closeup = 1;
        }
    }
    bfm_bedroom_field_step(s->field.field);
    if (!bfm_bedroom_motion_step(s->motion, action, &s->pose) ||
        !bfm_bedroom_motion_face(s->motion, s->tiles)) return 0;
    ++s->age;
    return 1;
}

int bfm_castle_audience_stage_draw(BfmCastleAudienceStage *s, const uint8_t flags[64],
                                   uint32_t pixels[240][320])
{
    if (!s || !flags || !pixels || !s->pose.pose) return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(s->field.field);
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(s->travel);
    BfmBedroomBody body = {
        .pose = s->pose.pose, .pose_bytes = 256, .heading = root->yaw, .pitch = s->pose.pitch,
        .face_word = s->pose.face_word, .visible_parts = bfm_village_field_visible(&s->pose, flags),
        .shadow_visible = root->shadow_visible, .blade_tick = s->age,
        .green_blade = (flags[129/8] >> (129%8)) & 1u,
        .lumina_scale_q12 = s->pose.lumina_scale_q12,
        .blade = &s->field.fusion, .lumina_guard = s->field.guard, .lumina_blade = s->field.blade
    };
    for (unsigned i = 0; i < 3; ++i) {
        body.position[i] = root->xyz[i];
        body.surface_normal[i] = root->normal[i];
        body.shadow_position[i] = root->shadow_xyz[i];
        body.shadow_normal[i] = root->shadow_normal[i];
    }
    if (!view ||
        !bfm_bedroom_figure_prepare(s->field.hero, view, &body, s->tiles, &pixels[0][0]) ||
        !bfm_bedroom_visitor_add(s->steward, s->field.hero, view, 0,
                                 s->home, s->heading, s->breathing[s->age % 10])) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    bfm_bedroom_field_draw(s->field.field, s->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, s->field.hero);
    return 1;
}

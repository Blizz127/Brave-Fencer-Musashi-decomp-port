/* SPDX-License-Identifier: MIT */
#include "southern_woods.h"
#include "woods_water.h"
#include "village_field.h"
#include "village_clock.h"
#include "village_lighting.h"
#include "village_sky.h"
#include "scene_tiles.h"
#include "forest_message_art.h"
#include "woods_sign_model.h"
#include "woods_caster.h"
#include "woods_award_effect.h"
#include "woods_fusion.h"
#include "woods_fusion_art.h"
#include "woods_shrink_message.h"
#include "woods_shrink_cast.h"
#include "woods_king.h"
#include "woods_king_reward.h"
#include "woods_body_notice.h"
#include "woods_lily.h"
#include "bedroom_effects.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>

struct BfmSouthernWoods {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    BfmVillageMotion *motion;
    BfmVillageField field;
    BfmVillageCamera camera;
    BfmVillageDaylight daylight;
    BfmWoodsWater water;
    uint16_t (*tiles)[1024];
    BfmWoodsSignModel *sign;
    BfmWoodsCaster *caster;
    BfmWoodsKing *king;
    BfmWoodsLilies *lilies;
    BfmWoodsKingReward king_reward;
    BfmWoodsBodyNotice body_notice;
    BfmWoodsFusion *fusion;
    BfmWoodsFusionFrame action;
    BfmWoodsShrinkCast *shrink;
    BfmWoodsShrinkFrame spell;
    unsigned sound;
    BfmWoodsAwardEffect apparition;
    const MusashiDiscMedia *disc;
    BfmSaveRecord *save;
    const uint8_t *main;
    size_t main_bytes;
    int award_open;
    BfmForestMessageArt *writing;
    const uint8_t *sign_text;
    size_t sign_bytes;
    unsigned age, open_delay;
};

static void follow(BfmSouthernWoods *woods, int entering)
{
    const BfmBedroomRoot *root = &bfm_village_motion_player(woods->motion)->root;
    BfmVillageCamera *cam = &woods->camera;
    const int32_t origin[3] = {0}, position[3] = {root->xyz[0], 0, root->xyz[2]};
    for (unsigned axis = 0; axis < 3; ++axis) {
        int target = root->xyz[axis] - (axis == 1 ? 128 : 0);
        int difference = target - cam->target[axis];
        int step = difference / 4;
        if (!step && difference) step = difference < 0 ? -1 : 1;
        cam->target[axis] = entering ? target : cam->target[axis] + step;
    }
    int bearing = bfm_village_motion_view_yaw(origin,position);
    bfm_village_camera_woods_orbit(cam, bearing, root->xyz[2], entering);
}

void bfm_southern_woods_free(BfmSouthernWoods *woods)
{
    if (!woods) return;
    bfm_forest_message_art_free(woods->writing);
    bfm_woods_fusion_free(woods->fusion);
    bfm_woods_shrink_cast_free(woods->shrink);
    bfm_woods_king_free(woods->king);
    bfm_woods_lilies_free(woods->lilies);
    bfm_woods_caster_free(woods->caster);
    bfm_woods_sign_model_free(woods->sign);
    bfm_village_motion_free(woods->motion);
    bfm_village_field_clear(&woods->field);
    free(woods);
}

BfmSouthernWoods *bfm_southern_woods_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *door, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    BfmBedroomEntry next = *entry;
    if (!bfm_castle_room_entry(disc,door,&next) || next.scene_member != 29) return NULL;
    BfmSouthernWoods *woods = calloc(1,sizeof *woods);
    if (!woods) return NULL;
    woods->bytes = bfm_read_archive_field_scene(disc,"SC03.CD",next.scene_member,woods->scene);
    woods->motion = bfm_village_motion_woods(woods->scene,woods->bytes,main,main_bytes);
    if (!woods->motion || !bfm_village_field_load_woods(&woods->field,disc,woods->scene,woods->bytes) ||
        !bfm_scene_read_tiles(disc,"MAIN.CD",10,tiles) ||
        !bfm_scene_read_tiles(disc,"SC03.CD",next.scene_member,tiles)) goto fail;
    bfm_woods_water_load(&woods->water, tiles);
    if (!bfm_woods_daylight_begin(&woods->daylight, woods->scene, woods->bytes,
                                  next.state.progress[0x37], tiles)) goto fail;
    const uint32_t base = UINT32_C(0x80128158);
    const size_t script = UINT32_C(0x8018bdec) - base;
    if (woods->bytes < script + 20) goto fail;
    const uint8_t *commands = woods->scene + script;
    if (u32le(commands) != 10 || u32le(commands+4) || u32le(commands+8) != 1 ||
        u32le(commands+16) != 2) goto fail;
    uint32_t address = u32le(commands + 12);
    if (address < base || address-base > woods->bytes-4) goto fail;
    woods->sign_text = woods->scene + (address-base);
    size_t available = woods->bytes - (address-base) - 4;
    /* Three rows of 36 cells, with two line breaks and a terminator. */
    const size_t capacity = 3*36 + 3;
    const uint8_t *end = memchr(woods->sign_text+4,0,available < capacity ? available : capacity);
    if (!end || memcmp(woods->sign_text,(const uint8_t[]){5,2,1,1},4)) goto fail;
    woods->sign_bytes = (size_t)(end - woods->sign_text) + 1;
    woods->sign = bfm_woods_sign_model_load(woods->scene,woods->bytes,UINT32_C(0x8018ea38));
    woods->writing = bfm_castle_message_art_load(disc,woods->scene,woods->bytes,tiles,0x5fd88);
    woods->caster = bfm_woods_caster_load(woods->scene, woods->bytes);
    woods->fusion = bfm_woods_fusion_open(disc, woods->scene, woods->bytes);
    woods->king = bfm_woods_king_open(woods->scene, woods->bytes, next.state.flags);
    woods->lilies = bfm_woods_lilies_open(disc, woods->scene, woods->bytes);
    woods->shrink = bfm_woods_shrink_cast_open(disc, woods->scene, woods->bytes);
    if (!woods->sign || !woods->writing || !woods->caster || !woods->fusion ||
        !woods->king || !woods->lilies || !woods->shrink ||
        !bfm_woods_king_reward_load(&woods->king_reward, disc, main, main_bytes) ||
        !bfm_woods_body_notice_load(&woods->body_notice, disc, woods->scene, woods->bytes)) goto fail;
    woods->disc = disc;
    woods->main = main;
    woods->main_bytes = main_bytes;
    woods->save = &entry->state;
    woods->action.tether_gauge = -1;
    woods->tiles = tiles;
    woods->camera.focal = 400;
    follow(woods,1);
    *entry = next;
    return woods;
fail:
    bfm_southern_woods_free(woods);
    return NULL;
}

const BfmVillagePlayer *bfm_southern_woods_player(const BfmSouthernWoods *woods)
{
    return bfm_village_motion_player(woods->motion);
}

int bfm_southern_woods_close(BfmSouthernWoods *woods)
{
    if (woods->award_open) {
        woods->award_open = 0;
        return bfm_woods_fusion_close_award(woods->fusion);
    }
    return bfm_village_motion_end_read(woods->motion);
}

int bfm_southern_woods_award_cue(const BfmSouthernWoods *woods)
{
    return woods->action.release_award;
}

unsigned bfm_southern_woods_sound(const BfmSouthernWoods *woods)
{
    return woods->sound;
}

int bfm_southern_woods_tether_gauge(const BfmSouthernWoods *woods)
{
    return woods->action.tether_gauge;
}

static int in_sign_reach(const BfmSouthernWoods *woods, const int16_t position[3])
{
    const int16_t *point = bfm_woods_sign_model_position(woods->sign);
    int64_t distance = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int delta = position[axis] - point[axis];
        distance += (int64_t)delta * delta;
    }
    return distance <= 64 * 64;
}

int bfm_southern_woods_step_field_at(BfmSouthernWoods *woods, BfmGameplayRun *run,
                                     BfmFieldInput field, unsigned actor_tick, BfmForestMessage **opened)
{
    if (opened) *opened = NULL;
    const BfmVillagePlayer *p = bfm_village_motion_player(woods->motion);
    int actors_running = !bfm_woods_fusion_awarding(woods->fusion);
    woods->sound = 0;
    bfm_woods_body_notice_step(&woods->body_notice);
    /* Ordinary outdoor time continues during the throw and absorption;
     * the acquisition pose and its three message pages suspend it. */
    if (run && p->controlled && actors_running)
        bfm_village_clock_step(woods->save, 0);
    bfm_village_daylight_step(&woods->daylight, woods->save->progress[0x37], woods->tiles);
    int16_t previous[3], endpoint[3];
    memcpy(previous,p->root.xyz,sizeof previous);
    BfmWoodsKingEvent king = {0};
    if (actors_running && !bfm_woods_king_step(woods->king, previous, woods->save->flags, &king))
        return 0;
    woods->sound = king.sound;
    if (king.body_reward) {
        unsigned delta = bfm_woods_king_reward_apply(&woods->king_reward, king.body_points, woods->save);
        if (delta) {
            bfm_woods_body_notice_start(&woods->body_notice, delta);
            woods->sound = BFM_BODY_LEVEL_SOUND;
        }
        if (run) {
            memcpy(run->banks.figure_prices, woods->save->figure_prices, sizeof run->banks.figure_prices);
            memcpy(run->banks.variables, woods->save->variables, sizeof run->banks.variables);
        }
    }
    if (run) memcpy(run->player.record, woods->save->progress, sizeof run->player.record);
    int may_read = field.square_pressed && !field.shoulder_held && !woods->action.blade_away;
    BfmWoodsShrinkInput spell = {
        .can_begin = p->controlled && actors_running && !woods->action.movement_locked &&
                     !woods->action.action && !woods->action.blade_away &&
                     !(may_read && in_sign_reach(woods,previous)) &&
                     bfm_village_motion_grounded(woods->motion),
        .circle_pressed = field.circle_pressed, .field_tick = actor_tick
    };
    int casting = run && bfm_woods_shrink_cast_claims(woods->shrink, &run->player, &spell);
    int locked = casting || woods->action.movement_locked;
    int yaw = bfm_village_motion_view_yaw(woods->camera.eye,woods->camera.target);
    if (!bfm_village_motion_step_woods(woods->motion, locked ? 0 : field.x, locked ? 0 : field.y, yaw,
                                      !locked && field.cross_pressed, !locked && field.cross_held,
                                      actors_running, woods->lilies)) return 0;
    memcpy(endpoint,p->root.xyz,sizeof endpoint);
    if (p->controlled) {
        for (unsigned pass = 0; pass < 2; ++pass) {
            int sign_hit = bfm_woods_sign_model_slide(woods->sign, previous, endpoint);
            int king_hit = bfm_woods_king_slide(woods->king, previous, endpoint, woods->age);
            if (!sign_hit && !king_hit) break;
        }
        if (memcmp(endpoint,p->root.xyz,sizeof endpoint) &&
            !bfm_village_motion_correct_contact(woods->motion,endpoint)) return 0;
        if (actors_running) bfm_woods_king_touch(woods->king,p->root.xyz);
    }
    if (opened && may_read && !locked && p->controlled && in_sign_reach(woods,p->root.xyz)) {
        const int16_t *point = bfm_woods_sign_model_position(woods->sign);
        if (bfm_village_motion_begin_read(woods->motion,point)) woods->open_delay = 4;
    }
    if (woods->open_delay && !--woods->open_delay) {
        if (!opened) return 0;
        *opened = bfm_field_message_borrow(woods->sign_text,woods->sign_bytes);
        if (!*opened) return 0;
    }
    follow(woods,0);
    if (!bfm_bedroom_field_set_focal(woods->field.field, woods->camera.focal)) return 0;
    bfm_bedroom_field_viewpoint(woods->field.field, woods->camera.eye, woods->camera.target, 1, 1);
    const BfmBedroomView *view = bfm_bedroom_field_view(woods->field.field);
    if (actors_running && !bfm_woods_caster_step(woods->caster, view, &p->root)) return 0;
    if (run) {
        BfmWoodsFusionInput cast = {
            .controllable=p->controlled && !woods->award_open && !casting &&
                          bfm_village_motion_grounded(woods->motion),
            .moving=p->root.xyz[0] != previous[0] || p->root.xyz[2] != previous[2],
            .shoulder=field.shoulder_held, .square_edge=field.square_pressed,
            .visible=bfm_village_field_visible(&p->pose, woods->save->flags), .eye=view->eye
        };
        if (!bfm_woods_fusion_step(woods->fusion, woods->caster, run, &run->player, &p->root,
                                   woods->age, &cast, &woods->action)) return 0;
        spell.eye = view->eye;
        spell.target_root = bfm_woods_king_shrink_root(woods->king);
        if (!bfm_woods_shrink_cast_step(woods->shrink, &run->player, &p->root, &spell,
                                       &woods->spell)) return 0;
        if (woods->spell.target_contact && !bfm_woods_king_shrink(woods->king)) return 0;
        if (woods->spell.sound) woods->sound = woods->spell.sound;
        if (woods->action.release_award) woods->sound = BFM_WOODS_SHRINK_SOUND;
        memcpy(woods->save->progress, run->player.record, sizeof woods->save->progress);
        if (woods->action.open_award) {
            if (!opened || *opened) return 0;
            *opened = bfm_woods_shrink_message_open(woods->scene, woods->bytes,
                                                   woods->main, woods->main_bytes);
            if (!*opened) return 0;
            woods->award_open = 1;
        }
    }
    if (!bfm_woods_award_effect_step(&woods->apparition, woods->disc, woods->scene, woods->bytes,
                                     woods->tiles, run && woods->action.release_award)) return 0;
    ++woods->age;
    return bfm_village_motion_face(woods->motion,woods->tiles) &&
           bfm_woods_shrink_cast_face(woods->shrink,woods->tiles);
}

int bfm_southern_woods_step_field(BfmSouthernWoods *woods, BfmGameplayRun *run,
                                  BfmFieldInput field, BfmForestMessage **opened)
{
    return bfm_southern_woods_step_field_at(woods, run, field, woods->age, opened);
}

int bfm_southern_woods_step(BfmSouthernWoods *woods, int x, int y,
                            int talk_edge, BfmForestMessage **opened)
{
    return bfm_southern_woods_step_field(woods, NULL,
        (BfmFieldInput){.x=x, .y=y, .square_pressed=talk_edge}, opened);
}

static void draw_actors(void *context, int depth)
{
    BfmSouthernWoods *woods = context;
    for (;;) {
        int sign = bfm_woods_sign_model_next_depth(woods->sign);
        int hero = bfm_bedroom_figure_next_depth(woods->field.hero);
        int next = sign > hero ? sign : hero;
        if (next == INT_MIN || next < depth) break;
        bfm_woods_sign_model_draw_to(woods->sign,next);
        bfm_bedroom_figure_draw_to(woods->field.hero,next);
    }
    if (depth == INT_MIN) bfm_bedroom_figure_draw_to(woods->field.hero,depth);
}

int bfm_southern_woods_draw(BfmSouthernWoods *woods, const uint8_t flags[64],
                            const BfmForestMessageFrame *window, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_southern_woods_player(woods);
    const size_t room = 0x8018ea38u - 0x80128158u;
    BfmBedroomPose displayed = p->pose;
    if (!bfm_woods_fusion_art_pose(woods->main, woods->main_bytes, woods->action.pose,
                                   woods->action.visible, woods->action.blade_away, &displayed)) return 0;
    bfm_woods_shrink_cast_pose(woods->shrink, &displayed);
    if (!bfm_bedroom_figure_lights(woods->field.hero, woods->spell.player_lit ?
                                   woods->spell.lamps : woods->daylight.lamps) ||
        !bfm_village_sky_draw_lit(woods->scene+room,woods->bytes-room,woods->daylight.sky,pixels) ||
        !bfm_village_field_prepare(&woods->field,&woods->camera,&p->root,&displayed,flags,
                                   woods->age,woods->tiles,&pixels[0][0])) return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(woods->field.field);
    if ((woods->spell.player_lit && !bfm_bedroom_figure_lights(woods->field.hero, woods->daylight.lamps)) ||
        !bfm_woods_lilies_add(woods->lilies, woods->field.hero, view) ||
        !bfm_woods_king_add(woods->king, woods->field.hero, view) ||
        !bfm_woods_caster_add(woods->caster, woods->field.hero, view) ||
        !bfm_woods_shrink_cast_append(woods->shrink, woods->field.hero, view) ||
        !bfm_woods_fusion_art_append(woods->field.hero, view, woods->action.blade,
                                     woods->action.trails, woods->action.bp_digits) ||
        !bfm_woods_fusion_art_lights(woods->field.hero, view, &woods->action) ||
        !bfm_woods_award_effect_append(&woods->apparition, woods->field.hero, view) ||
        !bfm_woods_sign_model_prepare(woods->sign,view,
                                      woods->tiles,&pixels[0][0])) return 0;
    bfm_woods_water_draw(&woods->water, woods->age, woods->field.field, woods->tiles,
                         &pixels[0][0], draw_actors, woods);
    if (window) bfm_forest_message_art_draw(woods->writing,window,pixels);
    bfm_woods_body_notice_draw(&woods->body_notice, woods->tiles, pixels);
    return 1;
}

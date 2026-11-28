/* SPDX-License-Identifier: MIT */
#include "village_mine_entrance.h"
#include "village_mine_props.h"
#include "village_mine_corridor_408.h"
#include "village_switchback_props.h"
#include "village_switchback_hit.h"
#include "village_mine_view.h"
#include "village_mine_shaft_view.h"
#include "village_mine_shaft_fan.h"
#include "woods_box_return.h"
#include "woods_3026_box_art.h"
#include "woods_3026_box_hit.h"
#include "bedroom_effects.h"
#include "field_rest.h"
#include "forest_message_art.h"
#include "scene_payload.h"
#include "village_clock.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    BfmMineShaftFan *fan;
    BfmWoods3026BoxArt *art;
    BfmWoods3026BoxHit contact;
    BfmWoodsBoxRoute route;
    BfmWoodsBoxUse use;
    int cleanup;
} MineShaft;

struct BfmVillageMineEntrance {
    uint8_t *scene;
    BfmVillageMotion *motion;
    BfmVillageMineView *view;
    BfmVillageMineProps *props;
    BfmVillageSwitchbackProps *switchback;
    BfmVillageMineWheels *wheels;
    MineShaft *shaft;
    BfmMineShaftSound shaft_sound;
    void *shaft_sound_context;
    BfmMineWheelSound wheel_sound;
    void *wheel_sound_context;
    BfmSwitchbackHit *hit;
    BfmMineLungeHit incoming;
    const uint8_t *main;
    size_t main_bytes;
    BfmSaveRecord *save;
    BfmForestMessage *window;
    BfmForestMessageArt *art;
    uint16_t (*tiles)[1024];
    unsigned age, orb;
    uint16_t room;
    int signs_unattended, departing;
};

const BfmVillagePlayer *bfm_village_mine_entrance_player(const BfmVillageMineEntrance *mine)
{
    return mine ? bfm_village_motion_player(mine->motion) : NULL;
}

int bfm_village_mine_entrance_rider(const BfmVillageMineEntrance *mine)
{
    return mine ? bfm_village_motion_mine_rider(mine->motion) : -1;
}

void bfm_village_mine_entrance_set_wheel_sound(BfmVillageMineEntrance *mine,
    BfmMineWheelSound callback, void *context)
{
    if (mine) {
        mine->wheel_sound = callback;
        mine->wheel_sound_context = context;
    }
}

void bfm_village_mine_entrance_set_shaft_sound(BfmVillageMineEntrance *mine,
    BfmMineShaftSound callback, void *context)
{
    if (mine) {
        mine->shaft_sound = callback;
        mine->shaft_sound_context = context;
    }
}

uint16_t bfm_village_mine_entrance_room(const BfmVillageMineEntrance *mine)
{
    return mine ? mine->room : 0;
}

const BfmVillageMotion *bfm_village_mine_entrance_departure(const BfmVillageMineEntrance *mine)
{
    return mine && bfm_village_motion_transfer(mine->motion) ? mine->motion : NULL;
}

void bfm_village_mine_entrance_free(BfmVillageMineEntrance *mine)
{
    if (!mine) return;
    bfm_forest_message_free(mine->window);
    bfm_forest_message_art_free(mine->art);
    bfm_village_motion_free(mine->motion);
    bfm_village_mine_props_free(mine->props);
    bfm_switchback_hit_free(mine->hit);
    bfm_village_switchback_props_free(mine->switchback);
    bfm_village_mine_wheels_free(mine->wheels);
    bfm_village_mine_view_free(mine->view);
    if (mine->shaft) {
        bfm_mine_shaft_fan_free(mine->shaft->fan);
        bfm_woods_3026_box_art_free(mine->shaft->art);
        free(mine->shaft);
    }
    free(mine->scene);
    free(mine);
}

BfmVillageMineEntrance *bfm_village_mine_entrance_open(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    return bfm_village_mine_entrance_open_at(disc, entry, main, main_bytes, tiles, 0);
}

BfmVillageMineEntrance *bfm_village_mine_entrance_open_at(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
    unsigned world_tick)
{
    if (!disc || !entry || !main || !main_bytes || !tiles) return NULL;
    uint32_t station = u32le(entry->state.destination);
    int entrance = entry->scene_member == 107 &&
        (station == UINT32_C(0x02003043) || station == UINT32_C(0x00013043));
    int corridor = entry->scene_member == 108 &&
        (station == UINT32_C(0x00003044) || station == UINT32_C(0x00013044));
    int shaft = entry->scene_member == 109 &&
        (station == UINT32_C(0x00003045) || station == UINT32_C(0x00013045) ||
         station == UINT32_C(0x00ff3045));
    if (!entrance && !corridor && !shaft) return NULL;
    BfmVillageMineEntrance *mine = calloc(1, sizeof *mine);
    if (!mine) return NULL;
    size_t bytes;
    if (corridor || shaft)
        mine->scene = bfm_scene_unpack_pac0(disc, "SC03.CD", entry->scene_member,
            BFM_FIELD_SCENE_CAPACITY, BFM_FIELD_SCENE_CAPACITY, &bytes);
    else
        mine->scene = bfm_village_field_data(disc, entry->scene_member, &bytes);
    if (!mine->scene) goto fail;
    mine->room = (uint16_t)station;
    mine->save = &entry->state;
    mine->tiles = tiles;
    mine->orb = mine->save->progress[0x48];
    if (corridor) {
        mine->age = world_tick;
        mine->motion = bfm_village_motion_switchback(mine->scene, bytes, main, main_bytes,
            entry->state.destination[2], disc);
        const BfmVillagePlayer *p = bfm_village_mine_entrance_player(mine);
        if (!p || !p->pose.pose) goto fail;
        mine->view = bfm_village_mine_corridor_view_open(disc, mine->scene, bytes, tiles);
        mine->switchback = bfm_village_switchback_props_open(mine->scene, bytes, p->root.xyz, world_tick);
        mine->main = main;
        mine->main_bytes = main_bytes;
        mine->hit = bfm_switchback_hit_open(disc, mine->scene, bytes, p->root.xyz);
        mine->wheels = bfm_village_mine_wheels_open(mine->scene, bytes);
        if (!mine->view || !mine->switchback || !mine->wheels || !mine->hit ||
            !bfm_village_motion_mine_orb(mine->motion, mine->orb, tiles)) goto fail;
        bfm_village_motion_mine_begin_field(mine->motion, mine->wheels);
        bfm_village_motion_woods_exit_script(mine->motion, 0);
        bfm_village_mine_corridor_view_follow(mine->view, p->root.xyz, p->root.yaw, 1, 0);
        return mine;
    }
    if (shaft) {
        mine->age = world_tick;
        mine->main = main;
        mine->main_bytes = main_bytes;
        mine->signs_unattended = 1;
        mine->shaft = calloc(1, sizeof *mine->shaft);
        if (!mine->shaft) goto fail;
        if (station == UINT32_C(0x00ff3045))
            mine->motion = bfm_village_motion_mine_shaft_saved(mine->scene, bytes, main,
                main_bytes, entry->state.destination, disc);
        else
            mine->motion = bfm_village_motion_mine_shaft(mine->scene, bytes, main, main_bytes,
                entry->state.destination[2], disc);
        const BfmVillagePlayer *p = bfm_village_mine_entrance_player(mine);
        if (!p || !p->pose.pose) goto fail;
        MineShaft *field = mine->shaft;
        mine->view = bfm_village_mine_shaft_view_open(disc, mine->scene, bytes, tiles);
        mine->props = bfm_village_mine_props_open_shaft(mine->scene, bytes);
        mine->art = bfm_castle_message_art_load(disc, mine->scene, bytes, tiles, 0x568c4);
        field->fan = bfm_mine_shaft_fan_open(mine->scene, bytes);
        field->art = bfm_woods_3026_box_art_load_shaft(mine->scene, bytes);
        if (!mine->view || !mine->props || !mine->art || !field->fan || !field->art ||
            !bfm_woods_box_route_open_shaft(&field->route, mine->scene, bytes) ||
            !bfm_woods_3026_box_hit_mount_shaft(mine->scene, bytes, &field->contact) ||
            !bfm_village_motion_mine_orb(mine->motion, mine->orb, tiles)) goto fail;
        bfm_village_motion_woods_exit_script(mine->motion, 0);
        bfm_village_motion_mine_props(mine->motion, mine->props, world_tick);
        bfm_village_mine_shaft_view_follow(mine->view, p->root.xyz);
        return mine;
    }
    /* This field program has an uncompressed room binding and a packed
     * group mesh. Its geometry and palette cannot be inherited from town. */
    const size_t room_pointer = UINT32_C(0x8018245c) - UINT32_C(0x80128158);
    if (bytes < room_pointer + 4 ||
        u32le(mine->scene + room_pointer) != UINT32_C(0x8018615c)) goto fail;
    unsigned story = u32le(mine->save->progress + 8);
    /* An optional moving resident can intercept a direction sign. Its live
     * position is needed to resolve a read on visits where it is present. */
    int resident_room = s16le(mine->save->variables + 10);
    mine->signs_unattended = story >= 471 || (!(resident_room & 0x100) && resident_room != 0x3043);
    mine->motion = bfm_village_motion_mine(mine->scene, bytes, main, main_bytes,
        entry->state.destination[2], story, disc);
    const BfmVillagePlayer *p = bfm_village_mine_entrance_player(mine);
    if (!p || !p->pose.pose) goto fail;
    mine->props = bfm_village_mine_props_open(mine->scene, bytes);
    mine->view = bfm_village_mine_view_open(disc, mine->scene, bytes, tiles);
    if (!mine->view || !mine->props) goto fail;
    mine->art = bfm_castle_message_art_load(disc, mine->scene, bytes, tiles, 0x5aa20);
    if (!mine->art || !bfm_village_motion_mine_orb(mine->motion, mine->orb, tiles)) goto fail;
    /* Contact selects the return row. Its station starts the outgoing tape
     * afterwards, independently of how long the host takes to prepare it. */
    bfm_village_motion_woods_exit_script(mine->motion, 0);
    bfm_village_motion_mine_props(mine->motion, mine->props, 0);
    bfm_village_mine_view_follow(mine->view, p->root.xyz, 1);
    return mine;
fail:
    bfm_village_mine_entrance_free(mine);
    return NULL;
}

static int step_shaft(BfmVillageMineEntrance *mine, BfmGameplayRun *run, BfmFieldInput input,
                      int pad_yaw, unsigned tick)
{
    MineShaft *shaft = mine->shaft;
    const BfmVillagePlayer *p = bfm_village_mine_entrance_player(mine);
    int16_t previous[3];
    memcpy(previous, p->root.xyz, sizeof previous);
    bfm_village_motion_mine_shaft_begin_field(mine->motion, shaft->fan, tick);
    if (shaft->cleanup) {
        shaft->cleanup = 0;
    } else {
        if (run && input.square_pressed && !mine->window && !shaft->use.route &&
            bfm_village_motion_can_examine(mine->motion)) {
            BfmForestTransform model;
            unsigned flags = mine->save->progress[0x23] & 0x80 ? 0x400u : 0;
            if (!bfm_bedroom_figure_actor_root(&p->root, p->pose.pitch, &model)) return 0;
            if (bfm_woods_3026_box_can_use(&shaft->contact, &input, &model, previous, flags, p->root.busy) &&
                !bfm_woods_box_use_begin(&shaft->use, &shaft->route, mine->main, mine->main_bytes)) return 0;
        }
        if (!shaft->use.route && !bfm_village_motion_step_mine_field(mine->motion, input.x, input.y,
            pad_yaw, input.cross_pressed, input.cross_held, run)) return 0;
    }
    BfmMineShaftFanFrame frame;
    if (!bfm_village_motion_mine_shaft_turn_fan(mine->motion, &frame)) return 0;
    if (mine->shaft_sound) {
        for (unsigned i = 0; i < frame.sound_count; ++i)
            mine->shaft_sound(mine->shaft_sound_context, frame.sound[i].command, frame.sound[i].value);
    }
    if (!bfm_village_motion_mine_shaft_box(mine->motion, &shaft->contact, tick, previous)) return 0;
    if (shaft->use.route) {
        unsigned cue;
        BfmForestMessage *page = NULL;
        if (!bfm_woods_box_use_step(&shaft->use, run, mine->save, &page, &cue)) return 0;
        if (page) mine->window = page;
        if (cue && mine->shaft_sound) mine->shaft_sound(mine->shaft_sound_context, cue, 0);
        shaft->cleanup = !shaft->use.route;
    }
    return 1;
}

int bfm_village_mine_entrance_step(BfmVillageMineEntrance *mine, BfmGameplayRun *run,
                                   BfmFieldInput input, unsigned actor_tick)
{
    if (!mine) return 0;
    const BfmVillagePlayer *p = bfm_village_mine_entrance_player(mine);
    BfmForestMessage *prior = mine->window;
    int using_box = mine->shaft && (mine->shaft->use.route || mine->shaft->cleanup);
    if (!using_box && !prior && mine->signs_unattended && input.square_pressed && bfm_village_motion_can_examine(mine->motion)) {
        int16_t point[3];
        mine->window = bfm_village_mine_props_read(mine->props, p->root.xyz, 1, NULL, point);
        if (mine->window && !bfm_village_motion_begin_read(mine->motion, point)) return 0;
    }
    int clock_running = !using_box && !mine->window && (p->controlled || bfm_village_motion_ingress_clock_running(mine->motion)) &&
                        !bfm_village_motion_transfer(mine->motion) &&
                        (mine->save->flags[127 / 8] & (1u << (127 % 8)));
    bfm_village_motion_mine_props(mine->motion, mine->props, actor_tick);
    if (mine->wheels) bfm_village_motion_mine_begin_field(mine->motion, mine->wheels);
    int pad_yaw = bfm_village_mine_view_pad_yaw(mine->view);
    int locked = mine->hit ? bfm_switchback_hit_step(mine->hit, mine->motion, run, mine->save,
                                                       input, (unsigned)pad_yaw, &mine->incoming) : 0;
    if (locked < 0) return 0;
    if (mine->shaft) {
        if (!step_shaft(mine, run, mine->window ? (BfmFieldInput){0} : input, pad_yaw, actor_tick)) return 0;
    } else if (!locked && !bfm_village_motion_step_mine_field(mine->motion, input.x, input.y, pad_yaw,
            input.cross_pressed, input.cross_held, run)) return 0;
    if (clock_running) {
        bfm_village_clock_step(mine->save, 0);
        bfm_field_rest_stamina(mine->save, 0);
    }
    if (mine->switchback) {
        int scripted = !p->controlled && !bfm_village_motion_ingress_clock_running(mine->motion);
        bfm_village_mine_corridor_view_follow(mine->view, p->root.xyz, p->root.yaw, 0, input.camera_pressed);
        int gain[BFM_MINE_WHEEL_COUNT];
        if (!bfm_village_motion_mine_turn_wheels(mine->motion, gain)) return 0;
        if (mine->wheel_sound) {
            for (unsigned i = 0; i < BFM_MINE_WHEEL_COUNT; ++i) {
                if (gain[i] > 0) mine->wheel_sound(mine->wheel_sound_context, 0x6dd, 0x1000u | (unsigned)gain[i], i);
                else if (gain[i] < 0) mine->wheel_sound(mine->wheel_sound_context, 4, 0x6dd, i);
            }
        }
        const int16_t *target = bfm_switchback_hit_target(mine->hit, mine->motion);
        if (!bfm_village_switchback_props_step_target(mine->switchback, p->root.xyz,
                scripted ? NULL : target, actor_tick, run, &mine->incoming)) return 0;
    } else if (mine->shaft) {
        bfm_village_mine_shaft_view_follow(mine->view, p->root.xyz);
    } else {
        bfm_village_mine_view_follow(mine->view, p->root.xyz, 0);
    }
    if (prior) {
        int box_page = mine->shaft && mine->shaft->use.wait_for_answer;
        if (box_page) {
            int direction = input.pressed_directions & 0x10 ? -1 : !!(input.pressed_directions & 0x40);
            if (bfm_field_message_choose(prior, direction, input.circle_pressed || input.triangle_pressed,
                                         input.cross_pressed) && mine->shaft_sound)
                mine->shaft_sound(mine->shaft_sound_context,
                    input.cross_pressed ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND, 0);
        }
        const BfmForestMessageFrame *frame = bfm_field_message_tick(prior, 0,
            input.cross_pressed, input.triangle_pressed);
        if (!frame || (frame->events & BFM_FOREST_VOICE_REQUEST)) return 0;
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
            if (box_page) bfm_woods_box_use_close(&mine->shaft->use, frame->choices ? frame->answer : 0);
            else if (!bfm_village_motion_end_read(mine->motion)) return 0;
            bfm_forest_message_free(prior);
            mine->window = NULL;
        }
    }
    if (mine->orb != mine->save->progress[0x48]) {
        mine->orb = mine->save->progress[0x48];
        if (!bfm_village_motion_mine_orb(mine->motion, mine->orb, mine->tiles)) return 0;
    }
    mine->age = actor_tick;
    return 1;
}

int bfm_village_mine_entrance_step_departure(BfmVillageMineEntrance *mine, BfmGameplayRun *run,
                                             unsigned actor_tick)
{
    if (!bfm_village_mine_entrance_departure(mine)) return 0;
    if (!mine->departing) {
        bfm_village_motion_woods_exit_script(mine->motion, 1);
        mine->departing = 1;
    }
    return bfm_village_mine_entrance_step(mine, run, (BfmFieldInput){0}, actor_tick);
}

unsigned bfm_village_mine_entrance_take_hit_sound(BfmVillageMineEntrance *mine)
{
    return mine ? bfm_switchback_hit_take_sound(mine->hit) : 0;
}

static int draw_switchback(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    BfmVillageMineEntrance *mine = context;
    return bfm_village_mine_wheels_draw(mine->wheels, figure, view) &&
           bfm_village_switchback_props_draw(mine->switchback, figure, view) &&
           bfm_switchback_hit_draw(mine->hit, figure, view);
}

static int draw_shaft(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    BfmVillageMineEntrance *mine = context;
    return bfm_mine_shaft_fan_draw(mine->shaft->fan, figure, view) &&
           bfm_village_mine_props_draw(mine->props, figure, view) &&
           bfm_woods_3026_box_art_draw(mine->shaft->art, figure, view, mine->shaft->use.openness);
}

int bfm_village_mine_entrance_draw(BfmVillageMineEntrance *mine, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_village_mine_entrance_player(mine);
    if (!p || !pixels || !bfm_village_motion_face(mine->motion, mine->tiles)) return 0;
    unsigned scale = mine->save->progress[0x23] & 0x80 ? BFM_HERO_MINI_SCALE : BFM_HERO_NORMAL_SCALE;
    if (mine->switchback) {
        BfmBedroomPose body = p->pose;
        unsigned visible = bfm_village_field_visible(&body, mine->save->flags);
        if (!bfm_switchback_hit_body(mine->hit, mine->main, mine->main_bytes, &body, &visible)) return 0;
        return bfm_village_mine_view_draw_parts(mine->view, &p->root, &body, mine->save->flags,
            mine->age, visible, scale, pixels, draw_switchback, mine);
    }
    if (mine->shaft)
        return bfm_village_mine_view_draw(mine->view, &p->root, &p->pose, mine->save->flags,
            mine->age, scale, pixels, draw_shaft, mine);
    return bfm_village_mine_view_draw(mine->view, &p->root, &p->pose, mine->save->flags,
        mine->age, scale, pixels, bfm_village_mine_props_draw, mine->props);
}

const BfmForestMessageFrame *bfm_village_mine_entrance_page(const BfmVillageMineEntrance *mine)
{
    return mine && mine->window ? bfm_forest_message_frame(mine->window) : NULL;
}

int bfm_village_mine_entrance_draw_window(const BfmVillageMineEntrance *mine, uint32_t pixels[240][320])
{
    if (!mine || !pixels) return 0;
    return !mine->window || bfm_forest_message_art_draw_placed(mine->art,
        bfm_forest_message_frame(mine->window), 0, 0, pixels);
}

/* SPDX-License-Identifier: MIT */
#include "bedroom.h"
#include "bedroom_award.h"
#include "bedroom_effects.h"
#include "bedroom_dust.h"
#include "bedroom_fade.h"
#include "bedroom_field.h"
#include "bedroom_figure.h"
#include "bedroom_motion.h"
#include "bedroom_script.h"
#include "bedroom_score.h"
#include "bedroom_reaction.h"
#include "bedroom_travel.h"
#include "bedroom_visitor.h"
#include "bedroom_visitors.h"
#include "scene_tiles.h"
#include "forest_message_art.h"
#include "forest_voice.h"
#include "forest_hud.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
typedef struct {
    int offset_x, portrait_on_right;
} BedroomWindowLayout;

typedef enum {
    NO_ITEM_ACTION, COLLECT_MOTION, DISPATCH_AWARD, SETUP_AWARD, APPROACH_AWARD, FACE_AWARD, READ_AWARD, LOWER_AWARD, TURN_BACK
} ItemAction;

struct BfmBedroom {
    uint8_t *scene, *main;
    size_t scene_bytes;
    uint16_t tiles[512][1024];
    BfmBedroomEntry entry;
    BfmBedroomScript script;
    BfmBedroomField *field;
    BfmBedroomMotion *motion;
    BfmBedroomFigure *figure;
    BfmBedroomTravel *travel;
    BfmBedroomVisitor *visitor_models;
    BfmBedroomVisitors *visitors;
    BfmBedroomReaction reaction;
    BfmBedroomFade fade;
    BfmForestMessage *message;
    BfmForestMessageArt *message_art;
    ForestVoice *voice;
    const MusashiDiscMedia *disc;
    BfmForestScore *scores[2], *prepared_score, *current_score;
    unsigned score_count, effect_request;
    BfmBedroomEffects *effects;
    uint32_t room_sound_group;
    BfmForestHud *hud;
    BfmForestStatus status;
    ItemAction item_action;
    uint32_t award_text;
    unsigned previous_yaw, ray_age;
    int rays_active, running;
    BfmBedroomDust dust;
    unsigned generation, voice_ready_field, message_closed, look_count;
    int waking, notice_pending, visitor_window;
    BedroomWindowLayout player_layout, visitor_layout;
    BfmBedroomAction requested, pending;
    BfmBedroomPose pose;
    int camera_actor[3];
    unsigned update, presented, drain;
    uint32_t frames[3][240][320];
};

static unsigned half(const uint8_t *p)
{
    return p[0] | ((unsigned)p[1] << 8);
}
static uint32_t word(const uint8_t *p)
{
    return half(p) | ((uint32_t)half(p + 2) << 16);
}
static const uint8_t *data_range(const BfmBedroom *room, uint32_t address, size_t size)
{
    if (address < scene_base || size > room->scene_bytes ||
        address - scene_base > room->scene_bytes - size) return NULL;
    return room->scene + (address - scene_base);
}
void bfm_bedroom_free(BfmBedroom *room)
{
    if (!room) return;
    for (unsigned i = 0; i < room->score_count; ++i) bfm_forest_score_free(room->scores[i]);
    bfm_forest_message_free(room->message);
    bfm_forest_message_art_free(room->message_art);
    bfm_forest_voice_free(room->voice);
    bfm_forest_hud_free(room->hud);
    bfm_bedroom_effects_free(room->effects);
    bfm_bedroom_field_free(room->field);
    bfm_bedroom_motion_free(room->motion);
    bfm_bedroom_figure_free(room->figure);
    bfm_bedroom_travel_free(room->travel);
    bfm_bedroom_visitors_free(room->visitors);
    bfm_bedroom_visitor_free(room->visitor_models);
    free(room->main);
    free(room->scene);
    free(room);
}
BfmBedroom *bfm_bedroom_open(const MusashiDiscMedia *disc, const BfmSaveRecord *record)
{
    BfmBedroomEntry entry;
    if (!bfm_bedroom_entry(disc, record, &entry)) return NULL;
    BfmBedroom *room = calloc(1, sizeof *room);
    if (!room) return NULL;
    room->disc = disc;
    room->entry = entry;
    room->waking = 1;
    room->scene = malloc(BFM_FIELD_SCENE_CAPACITY);
    if (!room->scene) goto failed;
    room->scene_bytes = bfm_read_field_scene(disc, entry.scene_member, room->scene);
    const uint8_t *root = data_range(room, 0x80182364u, 4);
    if (!root) goto failed;
    uint32_t address = word(root);
    const uint8_t *description = data_range(room, address, 0xcc);
    if (!description) goto failed;
    const uint8_t *placement = data_range(room, word(description + 0xb4), 20);
    if (!placement || half(placement + 6) != 360) goto failed;
    for (unsigned axis = 0; axis < 3; ++axis)
        room->camera_actor[axis] = (int16_t)half(placement + 2 * axis);
    room->field = bfm_bedroom_field_load(room->scene, room->scene_bytes, address);
    size_t main_bytes;
    room->main = bfm_bedroom_player_data(disc, &main_bytes);
    size_t sound_row = 0x800d34b0u - 0x800cedf8u;
    if (!room->main || !room->field || main_bytes < sound_row + 4) goto failed;
    room->room_sound_group = word(room->main+sound_row);
    if (!bfm_hero_run_smoke_load(&room->dust.sprite,room->main,main_bytes)) goto failed;
    room->motion = bfm_bedroom_motion_open(room->scene, room->scene_bytes, room->main, main_bytes);
    room->figure = bfm_bedroom_figure_load(disc, room->scene, room->scene_bytes, address);
    room->travel = bfm_bedroom_travel_open(room->scene, room->scene_bytes);
    room->visitors = bfm_bedroom_visitors_open(room->scene, room->scene_bytes);
    room->visitor_models = bfm_bedroom_visitor_load(room->scene, room->scene_bytes);
    if (!room->motion || !room->figure || !room->travel || !room->visitors || !room->visitor_models ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, room->tiles) ||
        !bfm_scene_read_tiles(disc, "SC01.CD", entry.scene_member, room->tiles) ||
        !bfm_bedroom_script_begin(&room->script, room->scene, room->scene_bytes)) goto failed;
    room->message_art = bfm_bedroom_message_art_load(disc, room->scene, room->scene_bytes, room->tiles);
    if (!room->message_art) goto failed;
    room->voice = bfm_bedroom_voice_load(disc);
    room->hud = bfm_bedroom_hud_load(disc, room->scene, room->scene_bytes, room->tiles);
    if (!room->hud) goto failed;
    room->entry.state.progress[8] = 10;
    uint8_t *p = room->entry.state.progress;
    p[0x3c] = p[0x3a];
    p[0x3d] = p[0x3b];
    unsigned bp = half(p + 0x3e);
    p[0x40] = (uint8_t)(bp < 100 ? bp : 100);
    p[0x41] = 0;
    memcpy(room->status.record, p, sizeof room->status.record);
    if (p[0x42] >= 7 || !bfm_bedroom_orb_palette(room->motion, p[0x48], room->tiles)) goto failed;
    bfm_bedroom_fade_begin(&room->fade);
    /* The early-story actor starts its program with progress 10 and clears
     * four transient scene flags in the restored live record. */
    room->entry.state.variables[226] = 0;
    room->entry.state.flags[303 / 8] |= (uint8_t)(1u << (303 % 8));
    for (unsigned flag = 293; flag <= 296; ++flag)
        room->entry.state.flags[flag >> 3] &= (uint8_t)~(1u << (flag & 7));
    return room;
failed:
    bfm_bedroom_free(room);
    return NULL;
}

static int apply(void *context, BfmBedroomEvent event, uint32_t operand)
{
    BfmBedroom *room = context;
    switch (event) {
    case BFM_BED_CAMERA_CUT:
    case BFM_BED_CAMERA_MOVE:
        return bfm_bedroom_field_camera(room->field, room->scene, room->scene_bytes,
                         operand, room->camera_actor, event == BFM_BED_CAMERA_CUT);
    case BFM_BED_REST: room->pending = BFM_BEDROOM_REST; break;
    case BFM_BED_SHOW_FUSION: room->pending = BFM_BEDROOM_SHOW_FUSION; break;
    case BFM_BED_LIE: room->requested = BFM_BEDROOM_LIE; break;
    case BFM_BED_RISE: room->requested = BFM_BEDROOM_RISE; break;
    case BFM_BED_YAWN:
        return bfm_bedroom_motion_step(room->motion, BFM_BEDROOM_YAWN, &room->pose);
    case BFM_BED_UPRIGHT: room->pending = BFM_BEDROOM_UPRIGHT; break;
    case BFM_BED_TURN: return bfm_bedroom_travel_turn(room->travel, operand & 4095u, room->waking);
    case BFM_BED_WALK: return bfm_bedroom_travel_walk(room->travel, (int16_t)operand,
                                                      (int16_t)(operand >> 16), room->waking);
    case BFM_BED_RETREAT:
        return bfm_bedroom_travel_retreat(room->travel, (int16_t)operand);
    case BFM_BED_NOTICE:
        room->notice_pending = 1;
        break;
    case BFM_BED_VISITORS_ENTER:
        room->camera_actor[0] = 136;
        room->camera_actor[2] = 96;
        return bfm_bedroom_visitors_spawn(room->visitors);
    case BFM_BED_RIBSON_SPEAKS:
        bfm_bedroom_visitors_request(room->visitors, BFM_VISITOR_RIBSON_SPEAK);
        break;
    case BFM_BED_RIBSON_RESTS:
        bfm_bedroom_visitors_request(room->visitors, BFM_VISITOR_RIBSON_REST);
        break;
    case BFM_BED_BUTLER_SPEAKS:
        bfm_bedroom_visitors_request(room->visitors, BFM_VISITOR_BUTLER_SPEAK);
        break;
    case BFM_BED_BUTLER_RESTS:
        bfm_bedroom_visitors_request(room->visitors, BFM_VISITOR_BUTLER_REST);
        break;
    case BFM_BED_SOUND_CHANGE:
        room->current_score = NULL;
        if (room->score_count >= sizeof room->scores / sizeof *room->scores) return 0;
        room->prepared_score = bfm_bedroom_score_load(room->disc, operand);
        if (!room->prepared_score) return 0;
        room->scores[room->score_count++] = room->prepared_score;
        break;
    case BFM_BED_SOUND_READY:
        room->current_score = room->prepared_score;
        return room->current_score != NULL;
    case BFM_BED_SOUND_FADE:
        room->current_score = NULL;
        break;
    case BFM_BED_HANDOVER: return bfm_bedroom_visitors_handover(room->visitors);
    case BFM_BED_COLLECT:
        if (room->item_action) return 0;
        room->item_action = COLLECT_MOTION;
        room->requested = BFM_BEDROOM_COLLECT;
        break;
    case BFM_BED_AWARD: {
        const uint8_t *descriptor = data_range(room, operand, 28);
        if (!descriptor || room->item_action || word(descriptor) != 1 ||
            word(descriptor + 4) || word(descriptor + 8) ||
            word(descriptor + 12) || word(descriptor + 16) || word(descriptor + 20))
            return 0;
        if (!room->effects) room->effects = bfm_bedroom_effects_load(room->disc, room->entry.scene_member, room->room_sound_group);
        if (!room->effects) return 0;
        room->award_text = word(descriptor + 24);
        if (!data_range(room, room->award_text, 1)) return 0;
        room->previous_yaw = bfm_bedroom_travel_root(room->travel)->yaw;
        room->item_action = DISPATCH_AWARD;
        break;
    }
    case BFM_BED_HUD_SHOW: bfm_forest_hud_show(room->hud); break;
    case BFM_BED_HUD_HIDE: bfm_bedroom_hud_hide(room->hud); break;
    case BFM_BED_VISITOR_LAYOUT:
    case BFM_BED_PLAYER_LAYOUT: {
        BedroomWindowLayout *layout = event == BFM_BED_VISITOR_LAYOUT ?
                                      &room->visitor_layout : &room->player_layout;
        layout->offset_x = (int16_t)operand;
        layout->portrait_on_right = (operand >> 16) != 0;
        break;
    }
    case BFM_BED_CUE:
    case BFM_BED_WINDOW_POSITION:
        break;
    case BFM_BED_WINDOW:
    case BFM_BED_VISITOR_WINDOW: {
        /* The room owns this bounded dialogue stream. Printing stops at its close command. */
        const uint8_t *program = data_range(room, operand, 1);
        if (!program || room->message) return 0;
        size_t bytes = room->scene_bytes - (size_t)(program - room->scene);
        room->visitor_window = event == BFM_BED_VISITOR_WINDOW;
        room->message = bfm_field_message_borrow(program, bytes);
        return room->message != NULL;
    }
    }
    return 1;
}

typedef struct {
    BfmBedroom *room;
    const BfmBedroomView *view;
    const BfmBedroomRoot *root;
    uint32_t *pixels;
    int effects_depth[4];
} RoomDraw;

static void draw_actors_to(void *context, int depth)
{
    RoomDraw *draw = context;
    for (;;) {
        unsigned next = 0;
        for (unsigned i=1; i<4; ++i)
            if (draw->effects_depth[i] > draw->effects_depth[next]) next = i;
        int at = draw->effects_depth[next];
        if (at == INT_MIN || at < depth) break;
        bfm_bedroom_figure_draw_to(draw->room->figure, at+1);
        if (next) bfm_bedroom_puff_draw(&draw->room->dust,next-1,draw->view,
                                        draw->room->tiles,(uint32_t (*)[320])draw->pixels);
        else bfm_bedroom_reaction_draw(&draw->room->reaction, draw->view, draw->root->xyz,
                                       draw->room->tiles, draw->pixels);
        draw->effects_depth[next] = INT_MIN;
    }
    bfm_bedroom_figure_draw_to(draw->room->figure, depth);
}

static int render_scene(BfmBedroom *room, const BfmBedroomPose *pose, uint32_t pixels[240][320])
{
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x)
            pixels[y][x] = UINT32_C(0xff000000);
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(room->travel);
    const BfmBedroomView *view = bfm_bedroom_field_view(room->field);
    if (pose->pose) {
        BfmBedroomBody body = { .pose = pose->pose, .pose_bytes = 256,
            .heading = root->yaw, .pitch = pose->pitch, .shadow_visible = root->shadow_visible,
            .face_word = pose->face_word };
        /* Flag 128 owns the second sword. Flag 303 suppresses the hand model
         * while the early-story room controls Musashi. */
        body.visible_parts = UINT32_C(0x7fffffff) &
                             ~((UINT32_C(1) << 22) | (UINT32_C(1) << 25));
        if (pose->show_weapon && !(room->entry.state.flags[303 / 8] & (1u << (303 % 8)))) {
            body.visible_parts &= ~(UINT32_C(1) << 21);
            body.visible_parts |= UINT32_C(1) << 22;
        }
        if (!(room->entry.state.flags[128 / 8] & 1))
            body.visible_parts &= ~(UINT32_C(1) << 24);
        for (unsigned axis = 0; axis < 3; ++axis) {
            body.position[axis] = root->xyz[axis];
            body.surface_normal[axis] = root->normal[axis];
            body.shadow_position[axis] = root->shadow_xyz[axis];
            body.shadow_normal[axis] = root->shadow_normal[axis];
        }
        if (!bfm_bedroom_figure_prepare(room->figure, view, &body, room->tiles, &pixels[0][0])) return 0;
    }
    for (unsigned i = 0; i < 2; ++i) {
        const BfmBedroomVisitorPose *guest = bfm_bedroom_visitors_pose(room->visitors, i);
        if (guest && guest->visible &&
            !bfm_bedroom_visitor_add(room->visitor_models, room->figure, view, i,
                    guest->position, guest->heading, guest->pose_address)) return 0;
    }
    RoomDraw draw = {.room=room,.view=view,.root=root,.pixels=&pixels[0][0]};
    draw.effects_depth[0] = bfm_bedroom_reaction_depth(&room->reaction,view,root->xyz);
    for (unsigned i=0; i<3; ++i)
        draw.effects_depth[i+1] = bfm_bedroom_puff_depth(&room->dust.puffs[i],view);
    bfm_bedroom_field_draw(room->field, room->tiles, draw.pixels,
                           pose->pose ? draw_actors_to : NULL, &draw);
    if (!room->waking && room->message) {
        const BedroomWindowLayout *layout = room->visitor_window ?
                                            &room->visitor_layout : &room->player_layout;
        int anchor = (layout->portrait_on_right ? 159 : 160) + layout->offset_x;
        /* The shared prompt blinks against the field that presents this frame. */
        BfmForestMessageFrame frame = *bfm_forest_message_frame(room->message);
        if ((room->update + 2) & 4) frame.inline_prompt = 0;
        bfm_bedroom_message_art_draw_side(room->message_art, &frame, anchor,
                                         layout->portrait_on_right, pixels);
    }
    bfm_forest_hud_draw(room->hud, &room->status, pixels);
    if (room->rays_active)
        bfm_bedroom_award_draw(data_range(room, 0x801855f8u, 32), view, root->xyz, room->ray_age, pixels);
    bfm_bedroom_fade_draw(&room->fade, pixels);
    return 1;
}

int bfm_bedroom_step_controls(BfmBedroom *room, int confirm_edge, int reveal_edge,
                              int direction, int accept_edge)
{
    room->effect_request = 0;
    int result = BFM_BEDROOM_ADVANCED;
    if (room->message_closed) {
        if (room->drain < 2) {
            room->presented = (room->presented + 1) % 3;
            ++room->drain;
        }
        return 1;
    }
    const BfmForestMessageFrame *previous = bfm_forest_message_frame(room->message);
    int window_blocks = previous && previous->focused;
    /* A room event can create a later actor slot immediately. An action
     * queued on the already-updated player starts on its next pass. */
    if (room->notice_pending) {
        if (!bfm_bedroom_reaction_start(&room->reaction, room->scene, room->scene_bytes)) return 0;
        room->notice_pending = 0;
    }
    unsigned old_status = bfm_forest_voice_status(room->voice);
    bfm_forest_voice_step(room->voice);
    if (old_status == BFM_FOREST_VOICE_PENDING &&
        bfm_forest_voice_status(room->voice) == BFM_FOREST_VOICE_PLAYING)
        room->voice_ready_field = 2 * room->update;
    BfmBedroomAction speech_action = BFM_BEDROOM_NONE;
    if (room->message) {
        int choice = bfm_field_message_choose(room->message, direction, confirm_edge && !accept_edge, accept_edge);
        if (choice) room->effect_request = accept_edge ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND;
        const BfmForestMessageFrame *f = bfm_field_message_tick(room->message,
                                     bfm_forest_voice_status(room->voice), confirm_edge, reveal_edge);
        if (!f) return 0;
        if (f->events & BFM_FOREST_VOICE_STOP) {
            bfm_forest_voice_stop(room->voice);
            result = BFM_BEDROOM_VOICE_STOPPED;
        }
        if (f->events & BFM_FOREST_VOICE_REQUEST) {
            if (bfm_forest_voice_request(room->voice, f->voice_bank, f->voice_cue))
                ++room->generation;
        }
        /* The room polls four game flags set by this window's script. Musashi
         * returns to his idle stance, looks in three directions for sixteen
         * updates each, faces the doorway, then gestures as he speaks. */
        unsigned flags = f->actor_flags;
        if (flags & (1u << 0)) speech_action = BFM_BEDROOM_REST;
        if (flags & (1u << 1)) room->look_count = 1;
        if (flags & (1u << 4)) {
            room->look_count = 0;
            bfm_bedroom_travel_face_point(room->travel, 32, -128);
        }
        if (room->look_count) {
            unsigned beat = (room->look_count - 1) / 16;
            bfm_bedroom_travel_look(room->travel, beat == 0 ? 3584 : beat == 1 ? 384 : 4032);
            if (room->look_count < 49) ++room->look_count;
        }
        if (flags & (1u << 2)) speech_action = BFM_BEDROOM_SEARCH;
        if (flags & (1u << 3)) speech_action = BFM_BEDROOM_SHOW_FUSION;
        if (f->events & BFM_FOREST_MESSAGE_CLOSED) {
            if (f->choices) room->script.answer = f->answer;
            if (room->item_action == READ_AWARD) {
                room->requested = BFM_BEDROOM_LOWER;
                room->item_action = LOWER_AWARD;
            }
            room->message_closed = bfm_bedroom_script_at_departure(&room->script);
            bfm_forest_message_free(room->message);
            room->message = NULL;
            room->waking = 0;
        }
    }
    const BfmBedroomView *view = bfm_bedroom_field_view(room->field);
    const BfmBedroomRoot *before_move = bfm_bedroom_travel_root(room->travel);
    int16_t run_origin[3];
    memcpy(run_origin,before_move->xyz,sizeof run_origin);
    if (room->update && !bfm_bedroom_travel_step(room->travel, view->eye, view->target))
        return 0;
    if (room->update) bfm_bedroom_field_step(room->field);
    if (room->update) bfm_bedroom_fade_step(&room->fade);
    if (room->rays_active && ++room->ray_age > 25) room->rays_active = 0;
    if (room->item_action == DISPATCH_AWARD) room->item_action = SETUP_AWARD;
    else if (room->item_action == SETUP_AWARD) room->item_action = APPROACH_AWARD;
    else if (room->item_action == APPROACH_AWARD) room->item_action = FACE_AWARD;
    else if (room->item_action == FACE_AWARD && bfm_bedroom_travel_turn_to_point(room->travel,
                                                                            view->eye[0], view->eye[2])) {
        if (!apply(room, BFM_BED_WINDOW, room->award_text)) return 0;
        speech_action = BFM_BEDROOM_RECEIVE;
        room->item_action = READ_AWARD;
        room->effect_request = BFM_ROOM_AWARD_SOUND;
        room->rays_active = 1;
        room->ray_age = 0;
    } else if (room->item_action == TURN_BACK) {
        unsigned yaw = bfm_bedroom_travel_root(room->travel)->yaw;
        if (yaw == room->previous_yaw) room->item_action = NO_ITEM_ACTION;
        else bfm_bedroom_travel_look(room->travel, (int)room->previous_yaw);
    }
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(room->travel);
    int emit = root->locomotion_edge == BFM_TRAVEL_WALK ||
               (room->running && !(room->update & 7) && root->locomotion_edge == BFM_TRAVEL_NONE);
    if (room->running && root->locomotion_edge == BFM_TRAVEL_STAND) emit = -1;
    bfm_bedroom_dust_step(&room->dust,emit < 0 ? root->xyz : run_origin,root->yaw,view->eye,emit);
    if (root->locomotion_edge == BFM_TRAVEL_WALK) room->running = 1;
    else if (root->locomotion_edge != BFM_TRAVEL_NONE || !root->busy) room->running = 0;
    BfmBedroomAction action = speech_action ? speech_action : room->pending;
    switch (root->locomotion_edge) {
    case BFM_TRAVEL_WALK: action = BFM_BEDROOM_WALK; break;
    case BFM_TRAVEL_FALL: action = BFM_BEDROOM_FALL; break;
    case BFM_TRAVEL_LAND: action = BFM_BEDROOM_LAND; break;
    case BFM_TRAVEL_STAND: action = BFM_BEDROOM_STAND; break;
    default: break;
    }
    /* The submitted part transforms belong to the previous actor update. */
    BfmBedroomPose displayed = room->pose;
    if (displayed.pose && !bfm_bedroom_motion_face(room->motion, room->tiles)) return 0;
    if (action || room->pose.pose) {
        if (!bfm_bedroom_motion_step(room->motion, action, &room->pose)) return 0;
    }
    if (room->pose.finished && (room->item_action == COLLECT_MOTION || room->item_action == LOWER_AWARD)) {
        room->item_action = room->item_action == LOWER_AWARD ? TURN_BACK : NO_ITEM_ACTION;
    }
    room->pending = room->requested;
    room->requested = BFM_BEDROOM_NONE;
    unsigned submitted = room->update % 3;
    if (!room->message_closed && !bfm_bedroom_script_tick(&room->script, !root->busy && !room->item_action,
                                                          window_blocks, apply, room)) return 0;
    if (!bfm_bedroom_visitors_step(room->visitors)) return 0;
    bfm_bedroom_reaction_step(&room->reaction);
    bfm_forest_hud_tick(room->hud);
    /* Scene-owned gestures submit freshly posed parts. Their first frame on
     * returning to the standing loop is also submitted by that callback. */
    if (displayed.draw_current || room->pose.draw_current || speech_action == BFM_BEDROOM_REST)
        displayed = room->pose;
    if (!render_scene(room, &displayed, room->frames[submitted])) return 0;
    /* Scene update, draw submission and scanout are separate game stages. */
    room->presented = (submitted + 1) % 3;
    ++room->update;
    return result;
}

BfmBedroomEffects *bfm_bedroom_effect(const BfmBedroom *room, unsigned *command)
{
    *command = room->effect_request;
    return room->effects;
}

BfmForestScore *bfm_bedroom_score(const BfmBedroom *room)
{
    return room ? room->current_score : NULL;
}

int bfm_bedroom_step_input(BfmBedroom *room, int confirm_edge, int reveal_edge)
{
    return bfm_bedroom_step_controls(room, confirm_edge, reveal_edge, 0, confirm_edge && !reveal_edge);
}

int bfm_bedroom_step(BfmBedroom *room)
{
    return bfm_bedroom_step_input(room, 0, 0);
}

int bfm_bedroom_draw(BfmBedroom *room, uint32_t pixels[240][320])
{
    memcpy(pixels, room->frames[room->presented], sizeof room->frames[0]);
    if (room->message && room->waking) {
        const BfmBedroomRoot *root = bfm_bedroom_travel_root(room->travel);
        int projection[3];
        if (!bfm_bedroom_project(bfm_bedroom_field_view(room->field), root->xyz, projection)) return 0;
        bfm_bedroom_message_art_draw(room->message_art, bfm_forest_message_frame(room->message),
                                     projection[0], pixels);
    }
    return 1;
}

int bfm_bedroom_endpoint(const BfmBedroom *room)
{
    return room->message_closed && room->drain == 2;
}

struct ForestVoice *bfm_bedroom_voice(BfmBedroom *room, unsigned *generation)
{
    *generation = room->generation;
    return room->voice;
}

unsigned bfm_bedroom_voice_ready_field(const BfmBedroom *room)
{
    return room->voice_ready_field;
}

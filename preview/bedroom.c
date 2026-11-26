/* SPDX-License-Identifier: MIT */
#include "bedroom.h"
#include "castle_cable.h"
#include "village_arrival.h"
#include "village_impact_sound.h"
#include "cable_sound.h"
#include "library_books.h"
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
    size_t scene_bytes, main_bytes;
    int library, command_room, gesture;
    unsigned command_delay, selected_command;
    BfmCastleCable *cable;
    BfmVillageArrival *village;
    BfmVillageImpactSound *landing_sound;
    unsigned village_updates, village_score_wait;
    BfmCableSound *slider_sound;
    int slider_event;
    unsigned village_trip, cable_tick;
    BfmForestBlade blade;
    unsigned transfer_wait, transition, arrival, entry_wait;
    BfmLibraryBooks *books;
    unsigned books_active, books_closing;
    uint32_t chapter_text;
    uint32_t (*books_background)[320];
    uint16_t tiles[512][1024];
    BfmBedroomEntry entry;
    uint8_t *story_flags;
    BfmGameplayRun local_run, *run;
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
    BfmForestScore *scores[5], *prepared_score, *current_score;
    unsigned score_count, effect_request;
    BfmBedroomEffects *effects, *knock;
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

void bfm_bedroom_bind_story_flags(BfmBedroom *room, uint8_t flags[64])
{
    if (!room || !flags || room->story_flags == flags) return;
    memcpy(flags, room->story_flags, sizeof room->entry.state.flags);
    room->story_flags = flags;
}

void bfm_bedroom_bind_run(BfmBedroom *room, BfmGameplayRun *run)
{
    if (!room || !run) return;
    bfm_bedroom_bind_story_flags(room, run->story_flags);
    room->run = run;
}

void bfm_bedroom_free(BfmBedroom *room)
{
    if (!room) return;
    for (unsigned i = 0; i < room->score_count; ++i) bfm_forest_score_free(room->scores[i]);
    bfm_forest_message_free(room->message);
    bfm_forest_message_art_free(room->message_art);
    bfm_forest_voice_free(room->voice);
    bfm_castle_cable_free(room->cable);
    bfm_village_arrival_free(room->village);
    bfm_village_impact_sound_free(room->landing_sound);
    bfm_cable_sound_free(room->slider_sound);
    bfm_forest_hud_free(room->hud);
    bfm_bedroom_effects_free(room->knock);
    bfm_bedroom_effects_free(room->effects);
    free(room->books_background);
    bfm_library_books_free(room->books);
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
    room->story_flags = room->entry.state.flags;
    room->run = &room->local_run;
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
    room->field = bfm_bedroom_field_load(room->scene, room->scene_bytes, address, 400);
    size_t main_bytes;
    room->main = bfm_bedroom_player_data(disc, &main_bytes);
    size_t sound_row = 0x800d34b0u - 0x800cedf8u;
    room->main_bytes = main_bytes;
    if (!room->main || !room->field || main_bytes < sound_row + 4) goto failed;
    room->room_sound_group = word(room->main+sound_row);
    if (!bfm_hero_run_smoke_load(&room->dust.sprite,room->main,main_bytes)) goto failed;
    room->motion = bfm_bedroom_motion_open(room->scene, room->scene_bytes, room->main, main_bytes, address);
    room->figure = bfm_bedroom_figure_load(disc, room->scene, room->scene_bytes, address,
                                            0x801841dcu, 0x80184f48u);
    room->travel = bfm_bedroom_travel_open(room->scene, room->scene_bytes, address, 0);
    room->visitors = bfm_bedroom_visitors_open(room->scene, room->scene_bytes);
    const uint32_t courtiers[] = {0x801c6b18u, 0x801c97c0u};
    room->visitor_models = bfm_bedroom_visitor_load(room->scene, room->scene_bytes,
                                                  courtiers, 2, 0x80183cc8u);
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
    if (p[0x42] >= 7 || !bfm_bedroom_orb_palette(room->motion, 0x80185b78u, p[0x48], room->tiles)) goto failed;
    bfm_bedroom_fade_begin(&room->fade);
    /* The early-story actor starts its program with progress 10 and clears
     * four transient scene flags in the restored live record. */
    room->entry.state.variables[226] = 0;
    room->story_flags[303 / 8] |= (uint8_t)(1u << (303 % 8));
    for (unsigned flag = 293; flag <= 296; ++flag)
        room->story_flags[flag >> 3] &= (uint8_t)~(1u << (flag & 7));
    return room;
failed:
    bfm_bedroom_free(room);
    return NULL;
}

static int enter_castle_room(BfmBedroom *r)
{
    BfmBedroomEntry entry=r->entry;
    memcpy(entry.state.flags, r->story_flags, sizeof entry.state.flags);
    int court = r->library;
    const uint8_t *destination=data_range(r,court ? 0x8018317cu : 0x80185e5cu,sizeof(entry.state.destination));
    if (!bfm_castle_room_entry(r->disc,destination,&entry)) return 0;
    unsigned entrance = entry.state.destination[2];
    bfm_library_books_free(r->books); r->books=NULL;
    free(r->books_background); r->books_background=NULL;
    r->books_closing=0;
    bfm_bedroom_field_free(r->field); r->field=NULL;
    bfm_bedroom_figure_free(r->figure); r->figure=NULL;
    bfm_bedroom_motion_free(r->motion); r->motion=NULL;
    bfm_bedroom_travel_free(r->travel); r->travel=NULL;
    bfm_bedroom_visitors_free(r->visitors); r->visitors=NULL;
    bfm_bedroom_visitor_free(r->visitor_models); r->visitor_models=NULL;
    bfm_forest_message_art_free(r->message_art); r->message_art=NULL;
    bfm_forest_voice_free(r->voice); r->voice=NULL;
    r->scene_bytes=bfm_read_field_scene(r->disc,entry.scene_member,r->scene);
    const uint8_t *root=data_range(r,court ? 0x8017f1d0u : 0x8017f744u,4);
    const uint8_t *d=root ? data_range(r,word(root),0xcc) : NULL;
    const uint8_t *actor=d ? data_range(r,word(d+0xb4),20) : NULL;
    if (!actor || half(actor+6)!=(court ? 354 : 366) || !bfm_scene_read_tiles(r->disc,"MAIN.CD",10,r->tiles) ||
        !bfm_scene_read_tiles(r->disc,"SC01.CD",entry.scene_member,r->tiles)) return 0;
    uint32_t address=word(root);
    for (unsigned i=0;i<3;++i) r->camera_actor[i]=(int16_t)half(actor+2*i);
    r->field=bfm_bedroom_field_load(r->scene,r->scene_bytes,address,court ? 300 : 400);
    r->travel=bfm_bedroom_travel_open(r->scene,r->scene_bytes,address,entrance);
    r->motion=bfm_bedroom_motion_open(r->scene,r->scene_bytes,r->main,r->main_bytes,address);
    r->figure=bfm_bedroom_figure_load(r->disc,r->scene,r->scene_bytes,address,
            court ? 0x80181048u : 0x801815bcu, court ? 0x80181db4u : 0x80182328u);
    r->visitors=bfm_bedroom_visitors_open(r->scene,r->scene_bytes);
    r->message_art=bfm_castle_message_art_load(r->disc,r->scene,r->scene_bytes,r->tiles,
                                               court ? 0x57794u : 0x57d08u);
    if (!bfm_castle_blade_load(&r->blade,r->scene,r->scene_bytes,court ? 0x5a000u : 0x5a574u) ||
        !r->field || !r->travel || !r->motion || !r->figure || !r->visitors || !r->message_art) return 0;
    if (court) {
        const int eye[3]={-190,-176,1604}, target[3]={-130,-232,1306};
        if (!bfm_bedroom_field_panorama(r->field,r->scene,r->scene_bytes,address)) return 0;
        bfm_bedroom_field_viewpoint(r->field,eye,target,1,1);
        if (entry.state.progress[8] < 5) entry.state.progress[8] = 5;
        memcpy(r->status.record, entry.state.progress, sizeof r->status.record);
        if (!bfm_bedroom_orb_palette(r->motion, 0x801829e4u, r->status.record[0x48], r->tiles)) return 0;
        r->command_delay=12;
        r->script.finished=1;
    } else {
        const uint32_t scribe=0x8019e198u;
        r->visitor_models=bfm_bedroom_visitor_load(r->scene,r->scene_bytes,&scribe,1,0x801810a8u);
        r->voice=bfm_library_voice_load(r->disc);
        const int target[3]={0,-275,-386};
        if (!r->visitor_models || !r->voice || !bfm_library_script_begin(&r->script,r->scene,r->scene_bytes) ||
            !bfm_bedroom_visitors_scribe(r->visitors,r->camera_actor,half(actor+12)) ||
            !bfm_bedroom_field_orbit(r->field,target,0xd8,400,168,1,12)) return 0;
    }
    memset(&r->pose,0,sizeof r->pose);
    memset(r->dust.puffs,0,sizeof r->dust.puffs);
    r->requested=r->pending=BFM_BEDROOM_NONE;
    const uint8_t *walk_duration = data_range(r, court ? 0x80182ddcu : 0x80183204u, 4);
    if (!walk_duration || !bfm_bedroom_motion_step(r->motion, BFM_BEDROOM_ENTER, &r->pose) ||
        !bfm_bedroom_travel_entrance(r->travel, word(walk_duration))) return 0;
    entry.state.flags[303 / 8] &= (uint8_t)~(1u << (303 % 8));
    memcpy(r->story_flags, entry.state.flags, sizeof entry.state.flags);
    r->entry=entry;
    r->library=!court;
    r->command_room=court;
    r->entry_wait=4;
    r->arrival=1;
    return 1;
}

static int apply(void *context, BfmBedroomEvent event, uint32_t operand)
{
    BfmBedroom *room = context;
    switch (event) {
    case BFM_BED_FAREWELL:
    case BFM_BED_NEGATIVE_REPLY:
        room->requested = event == BFM_BED_FAREWELL ? BFM_BEDROOM_FAREWELL : BFM_BEDROOM_NEGATIVE_REPLY;
        room->gesture = 1;
        break;
    case BFM_BED_CAMERA_RELEASE: {
        if (room->library) {
            const int library_target[3] = {0,-275,-386};
            return bfm_bedroom_field_orbit(room->field, library_target, 0xd8, 400, 168, 0, 6);
        }
        const int target[3] = {13, -130-130, 138-384};
        return bfm_bedroom_field_orbit(room->field, target, 0x50, 400, 0, 0, 6);
    }
    case BFM_BED_TRANSFER:
        room->transfer_wait = 17;
        break;
    case BFM_BED_BOOKS:
    case BFM_BED_BOOKS_RESUME:
        if (!room->library && !room->command_room) return 0;
        if (!room->books) {
            room->books = bfm_castle_menu_open(room->disc,room->scene,room->scene_bytes,room->command_room);
            room->books_background = malloc(sizeof room->frames[0]);
            if (!room->books || !room->books_background) return 0;
        }
        if (room->command_room) bfm_castle_hud_command(room->hud);
        bfm_library_books_resume(room->books, event == BFM_BED_BOOKS_RESUME);
        room->books_closing = 0;
        room->books_active = 1;
        break;
    case BFM_BED_PROJECT_VISITOR: {
        int16_t actor[3];
        for (unsigned i=0; i<3; ++i) actor[i] = (int16_t)room->camera_actor[i];
        const BfmBedroomView *view = bfm_bedroom_field_view(room->field);
        const BfmBedroomRoot *hero = bfm_bedroom_travel_root(room->travel);
        int guest_xy[3], hero_xy[3];
        if (!bfm_bedroom_project(view, actor, guest_xy) ||
            !bfm_bedroom_project(view, hero->xyz, hero_xy)) return 0;
        int right = guest_xy[0] >= hero_xy[0];
        const uint8_t *limits = data_range(room, 0x8017fed8u, 12);
        if (!limits) return 0;
        int scale = (right ? -1 : 1) * 8 * 904;
        int q = (guest_xy[0]-160) * 4096 / scale;
        if (q) {
            int lo = (int16_t)half(limits);
            int hi = (int16_t)half(limits+10);
            if (q > (int16_t)half(limits+2) && q < (int16_t)half(limits+8)) {
                lo = (int16_t)half(limits+4);
                hi = (int16_t)half(limits+6);
            }
            if (q < lo) q = lo;
            if (q > hi) q = hi;
        }
        /* The speech renderer accepts the speaker anchor, one pixel beyond
         * the reflected tail's right edge. Its left edge uses the anchor. */
        int product = q*scale;
        int offset = product/4096 - (product%4096 < 0);
        room->visitor_layout.offset_x = offset + (right ? 2 : 0);
        room->visitor_layout.portrait_on_right = right;
        break;
    }
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
        if (room->chapter_text) {
            const uint8_t *chapter = data_range(room, room->chapter_text, 1);
            if (!chapter || !bfm_field_message_insert_text(room->message, chapter,
                    room->scene_bytes - (size_t)(chapter-room->scene))) return 0;
            room->chapter_text = 0;
        }
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
        if (pose->show_weapon && !(room->story_flags[303 / 8] & (1u << (303 % 8)))) {
            if (room->library || room->command_room) {
                body.blade = &room->blade;
                body.blade_tick = room->update ^ 1u;
                body.green_blade = (room->story_flags[129 / 8] >> (129 % 8)) & 1u;
            }
            body.visible_parts &= ~(UINT32_C(1) << 21);
            body.visible_parts |= UINT32_C(1) << 22;
        }
        if (!(room->story_flags[128 / 8] & 1))
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
        if (room->library) {
            /* The library submits its prompt on bit 2 of the normal field
             * clock. BOOKS pauses that clock. Its phase relative to putting
             * the earned room on screen comes from the first reading. */
            frame.inline_prompt &= !!((room->update + 3u) & 4u);
        } else if ((room->update + 2) & 4) frame.inline_prompt = 0;
        bfm_bedroom_message_art_draw_side(room->message_art, &frame, anchor,
                                         layout->portrait_on_right, pixels);
    }
    bfm_forest_hud_draw(room->hud, &room->status, pixels);
    if (room->rays_active)
        bfm_bedroom_award_draw(data_range(room, 0x801855f8u, 32), view, root->xyz, room->ray_age, pixels);
    bfm_bedroom_fade_draw(&room->fade, pixels);
    return 1;
}

static int finish_castle_command(BfmBedroom *room)
{
    const uint8_t *choice = data_range(room,0x8018c2d4u+4*room->selected_command,4);
    if (!choice) return 0;
    uint32_t address = word(choice);
    int first_village = room->selected_command == 4 && !address &&
                        !(room->story_flags[181 / 8] & (1u << (181 % 8)));
    if (room->selected_command == 4 && !address)
        address = first_village ? 0x80182c74u : 0x80182d24u;
    if (address) {
        const uint8_t *door=data_range(room,address,sizeof room->entry.state.destination);
        if (!door) return 0;
        memcpy(room->entry.state.destination,door,sizeof room->entry.state.destination);
    }
    if (first_village) {
        room->story_flags[181 / 8] |= 1u << (181 % 8);
        room->entry.state.progress[8] = 10;
        uint8_t *clock = room->entry.state.progress + 0x18;
        clock[0] = clock[1] = 0;
        clock[2] = 0x1c;
        clock[3] = 2;
        room->village_trip=1;
        for (unsigned i=0;i<3;++i)
            if (i!=room->presented)
                memcpy(room->frames[i],room->frames[room->presented],sizeof room->frames[i]);
    }
    return 1;
}

/* Durations of the two field-loader passes on the earned US room path, in
 * 30 Hz room updates from the accepted Village command. */
enum { COURT_WIPE = 34, STATION_START = 168, LAST_STATION_UPDATE = 263, STATION_WIPE = 269, VILLAGE_PENDING = 359 };

static void advance_voice(BfmBedroom *room)
{
    unsigned old_status = bfm_forest_voice_status(room->voice);
    bfm_forest_voice_step(room->voice);
    if (old_status == BFM_FOREST_VOICE_PENDING &&
        bfm_forest_voice_status(room->voice) == BFM_FOREST_VOICE_PLAYING)
        room->voice_ready_field = 2 * room->update;
}

static const BfmForestMessageFrame *advance_message(BfmBedroom *room, int confirm_edge,
                                                     int reveal_edge, int *result)
{
    const BfmForestMessageFrame *frame = bfm_field_message_tick(room->message,
                                bfm_forest_voice_status(room->voice), confirm_edge, reveal_edge);
    if (!frame) return NULL;
    if (frame->events & BFM_FOREST_VOICE_STOP) {
        bfm_forest_voice_stop(room->voice);
        *result = BFM_BEDROOM_VOICE_STOPPED;
    }
    if ((frame->events & BFM_FOREST_VOICE_REQUEST) &&
        bfm_forest_voice_request(room->voice, frame->voice_bank, frame->voice_cue))
        ++room->generation;
    return frame;
}

static int village_step(BfmBedroom *room, int confirm_edge, int reveal_edge, int direction,
                        int accept_edge, int x, int y, int talk_edge)
{
    int result = BFM_BEDROOM_ADVANCED;
    int was_in_dialogue = room->message != NULL;
    advance_voice(room);
    if (room->message) {
        if (bfm_field_message_choose(room->message, direction, confirm_edge && !accept_edge, accept_edge))
            room->effect_request = accept_edge ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND;
        const BfmForestMessageFrame *frame = advance_message(room, confirm_edge, reveal_edge, &result);
        if (!frame) return 0;
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
            unsigned answer = frame->choices ? frame->answer : 0;
            bfm_forest_message_free(room->message);
            room->message = NULL;
            if (!bfm_village_arrival_message_closed(room->village, answer)) return 0;
        }
    }
    if (room->village_score_wait && --room->village_score_wait == 0) {
        if (!apply(room, BFM_BED_SOUND_READY, 0) || !bfm_village_arrival_continue(room->village))
            return 0;
    }
    BfmForestMessage *opened = NULL;
    if (!bfm_village_arrival_step_input(room->village, room->run, x, y,
                                        talk_edge && !was_in_dialogue, &opened)) return 0;
    if (opened) room->message = opened;
    if (room->story_flags != room->entry.state.flags)
        memcpy(room->story_flags, room->entry.state.flags, sizeof room->entry.state.flags);
    const BfmVillagePlayer *player = bfm_village_arrival_player(room->village);
    if (player->pose.knock_contact) {
        if (!room->knock) room->knock = bfm_village_knock_sound_load(room->disc);
        if (!room->knock) return 0;
        room->effect_request = BFM_VILLAGE_KNOCK_SOUND;
    }
    if (player->events & BFM_VILLAGE_CAMERA) {
        unsigned minute = half(room->entry.state.progress + 0x1a);
        if (!apply(room, BFM_BED_SOUND_CHANGE, minute >= 420 && minute < 1140 ? 0x0f : 0x10))
            return 0;
        /* The observed village handoff admits the waiting actor on the
         * 45th room update after the camera change and bank request. */
        room->village_score_wait = 45;
    }
    if (player->events & BFM_VILLAGE_OPEN_MESSAGE) {
        if (room->message) return 0;
        room->message = bfm_village_arrival_speech(room->village);
        if (!room->message) return 0;
    }
    if (player->events & BFM_VILLAGE_SHOW_HUD) bfm_forest_hud_show(room->hud);
    bfm_forest_hud_tick(room->hud);
    uint32_t (*frame)[320] = room->frames[room->presented];
    if (!bfm_village_arrival_draw(room->village, room->story_flags,
                room->message ? bfm_forest_message_frame(room->message) : NULL, frame)) return 0;
    memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
    bfm_forest_hud_draw(room->hud, &room->status, frame);
    room->presented = (room->presented + 1) % 3;
    ++room->village_updates;
    ++room->update;
    return result;
}

static int cable_trip_step(BfmBedroom *room)
{
    unsigned tick = ++room->cable_tick;
    if (tick == STATION_START) {
        room->cable=bfm_castle_cable_open(room->disc,&room->entry,room->main,room->main_bytes,room->tiles);
        room->slider_sound=bfm_cable_sound_load(room->disc);
        if (!room->cable || !room->slider_sound) return 0;
    }
    /* The station score becomes audible during black, about 150 ms before
     * the first slider note on the recorded US soundtrack. */
    if (tick == STATION_START-3 &&
        (!apply(room,BFM_BED_SOUND_CHANGE,0x32) || !apply(room,BFM_BED_SOUND_READY,0))) return 0;
    if (tick >= STATION_START && tick <= LAST_STATION_UPDATE) {
        if (!bfm_castle_cable_step(room->cable,room->story_flags,room->run,room->entry.state.destination,
                                  room->frames[room->presented])) return 0;
    }
    if (tick <= COURT_WIPE || (tick >= STATION_START && tick <= LAST_STATION_UPDATE+2))
        room->presented=(room->presented+1)%3;
    if (tick == STATION_START+1) room->slider_event=1;
    if (tick == STATION_START+77) room->slider_event=-1;
    if (tick == VILLAGE_PENDING) {
        if (room->story_flags != room->entry.state.flags)
            memcpy(room->entry.state.flags, room->story_flags, sizeof room->entry.state.flags);
        room->village = bfm_village_arrival_open(room->disc, &room->entry, room->main,
                                                room->main_bytes, room->tiles);
        if (!room->village) return 0;
        bfm_village_arrival_set_scene_tick(room->village, room->update);
        bfm_forest_voice_free(room->voice);
        room->voice = bfm_bedroom_voice_load(room->disc);
        room->landing_sound = bfm_village_impact_sound_load(room->disc);
        if (!room->voice || !room->landing_sound) return 0;
        room->current_score = NULL;
        bfm_bedroom_hud_hide(room->hud);
        memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
        memset(room->frames, 0, sizeof room->frames);
    }
    ++room->update;
    return BFM_BEDROOM_ADVANCED;
}

int bfm_bedroom_step_axes(BfmBedroom *room, int confirm_edge, int reveal_edge,
                          int direction, int held, int accept_edge, int back_edge, int x, int y, int talk_edge)
{
    room->effect_request = 0;
    room->slider_event = 0;
    int result = BFM_BEDROOM_ADVANCED;
    if (room->village) return village_step(room, confirm_edge, reveal_edge, direction,
                                            accept_edge, x, y, talk_edge);
    if (room->village_trip && room->cable_tick>=COURT_WIPE) return cable_trip_step(room);
    if (room->village_trip) {
        ++room->cable_tick;
        /* The courtyard exit keeps its ordinary player and HUD updates
         * active while Musashi walks to the mounted foreground waypoint. */
        if (room->cable_tick==1) {
            bfm_castle_hud_command_closed(room->hud);
            const uint8_t *point=data_range(room,0x80182e00u,8);
            if (!point || !bfm_bedroom_travel_walk(room->travel,(int16_t)half(point),
                                                   (int16_t)half(point+4),0)) return 0;
        }
        if (room->cable_tick==12) room->current_score=NULL;
    }
    if (room->books_active) {
        /* The constructor and setup precede the first interactive submission.
         * The field scanout queue presents the frozen room and modal frames. */
        unsigned submitted = room->presented;
        if (room->books_active == 1)
            memcpy(room->books_background, room->frames[(submitted+2)%3], sizeof room->frames[0]);
        memcpy(room->frames[submitted], room->books_background, sizeof room->frames[0]);
        if (room->books_closing) {
            room->books_active = 0;
            if (room->command_room) room->message_closed = 1;
        } else if (room->books_active <= 2) {
            ++room->books_active;
        } else {
            int selection = bfm_library_books_input(room->books, direction, held, accept_edge, back_edge,
                                      room->entry.state.variables[98], &room->effect_request);
            if (!bfm_library_books_draw(room->books, room->message_art, room->tiles,
                                        room->frames[submitted])) return 0;
            if (selection == BFM_COMMAND_SELECTED) {
                room->selected_command=bfm_castle_command_selection(room->books);
                room->books_closing=1;
            } else if (selection == BFM_BOOKS_QUIT) {
                room->script.book = 0;
                room->books_closing = 1;
            } else if (selection == BFM_BOOKS_SELECTED) {
                unsigned selection = bfm_library_books_selection(room->books);
                unsigned book = selection >> 8;
                unsigned topic = selection & 0xffu;
                /* The disc reserves five text pointers per book, including
                 * unused slots in shorter tables of contents. */
                if (!book || !topic || topic > 5) return 0;
                unsigned slot = (book - 1) * 5 + topic - 1;
                const uint8_t *entry = data_range(room, 0x8019f144u + 4 * slot, 4);
                if (!entry || !data_range(room, word(entry), 1)) return 0;
                room->script.book = selection;
                if (book == 1)
                    room->story_flags[232 / 8] |= 1u << (232 % 8);
                room->chapter_text = word(entry);
                room->books_closing = 1;
            }
        }
        room->presented = (submitted+1)%3;
        return result;
    }
    if (room->message_closed && !room->village_trip) {
        if (room->drain < 2) {
            room->presented = (room->presented + 1) % 3;
            ++room->drain;
            /* The field owner consumes COMMAND's result after the modal
             * display has handed both buffers back to the room. */
            if (room->command_room && room->drain == 2 && !finish_castle_command(room)) return 0;
        }
        return 1;
    }
    /* Room actor completion counts down sixteen to minus one. The old
     * room remains the display source through the field-loader wipe. */
    if (room->transfer_wait && !--room->transfer_wait) {
        room->transition=1;
        room->arrival=0;
        if (!room->library) room->entry.state.progress[8]=2;
    }
    if (room->transition) {
        unsigned wipe_start = room->library ? 43 : 41;
        if (room->library && room->transition == 2) {
            /* The library's door controller sends the player toward its
             * mounted exit waypoint as the room leaves ordinary control. */
            const uint8_t *point = data_range(room, 0x80183214u, 8);
            if (!point || !bfm_bedroom_travel_walk(room->travel,
                    (int16_t)half(point), (int16_t)half(point + 4), 0)) return 0;
        }
        /* The outgoing door allows the score to continue after relinquishing
         * room control. Its exit callback fades the active song when it
         * starts the screen wipe. */
        if (!room->library && room->transition == 41) room->current_score = NULL;
        /* The room requests the new bank once the old picture is black.
         * On the measured US transfer its first attack follows 42 room
         * updates later, two updates before the incoming field activates. */
        if (!room->library && room->transition == 126 && !apply(room, BFM_BED_SOUND_CHANGE, 0x3d)) return 0;
        if (!room->library && room->transition == 168 && !apply(room, BFM_BED_SOUND_READY, 0)) return 0;
        /* The library and castle approach share the already playing bank. */
        if (room->transition==(room->library ? 129u : 170u)) {
            if (!enter_castle_room(room)) return 0;
            room->transition=0;
        } else if (room->transition >= wipe_start) {
            ++room->transition;
            ++room->update;
            return result;
        } else ++room->transition;
    }
    const BfmForestMessageFrame *previous = bfm_forest_message_frame(room->message);
    int window_blocks = previous && previous->focused;
    /* A room event can create a later actor slot immediately. An action
     * queued on the already-updated player starts on its next pass. */
    if (room->notice_pending) {
        if (!bfm_bedroom_reaction_start(&room->reaction, room->scene, room->scene_bytes)) return 0;
        room->notice_pending = 0;
    }
    advance_voice(room);
    BfmBedroomAction speech_action = BFM_BEDROOM_NONE;
    if (room->message) {
        int choice = bfm_field_message_choose(room->message, direction, confirm_edge && !accept_edge, accept_edge);
        if (choice) room->effect_request = accept_edge ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND;
        const BfmForestMessageFrame *f = advance_message(room, confirm_edge, reveal_edge, &result);
        if (!f) return 0;
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
    /* The doorway animation keeps updating the player after the old field
     * camera relinquishes control. Its last submitted view survives the wipe. */
    if (room->update && !(room->library && room->transition > 4))
        bfm_bedroom_field_step(room->field);
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
    if (room->pose.finished) room->gesture = 0;
    if (room->pose.finished && (room->item_action == COLLECT_MOTION || room->item_action == LOWER_AWARD)) {
        room->item_action = room->item_action == LOWER_AWARD ? TURN_BACK : NO_ITEM_ACTION;
    }
    room->pending = room->requested;
    room->requested = BFM_BEDROOM_NONE;
    unsigned submitted = room->presented;
    int room_ready=1;
    if (room->arrival) {
        if (room->entry_wait) --room->entry_wait;
        if (room->arrival<25) ++room->arrival;
        if (room->entry_wait || root->busy) room_ready=0;
    }
    if (room->command_room && room_ready && room->command_delay) {
        if (room->command_delay == 12) bfm_forest_hud_show(room->hud);
        if (!--room->command_delay && !apply(room,BFM_BED_BOOKS,0)) return 0;
    }
    if (room_ready && !room->message_closed && !bfm_bedroom_script_tick(&room->script, !root->busy && !room->item_action && !room->gesture,
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

BfmVillageImpactSound *bfm_bedroom_landing(const BfmBedroom *room)
{
    if (room->village && (bfm_village_arrival_player(room->village)->events & BFM_VILLAGE_HIT))
        return room->landing_sound;
    return NULL;
}

BfmCableSound *bfm_bedroom_slider(const BfmBedroom *room, int *event)
{
    *event=room->slider_event;
    return room->slider_sound;
}

BfmBedroomEffects *bfm_bedroom_effect(const BfmBedroom *room, unsigned *command)
{
    *command = room->effect_request;
    return *command == BFM_VILLAGE_KNOCK_SOUND ? room->knock : room->effects;
}

BfmForestScore *bfm_bedroom_score(const BfmBedroom *room)
{
    return room ? room->current_score : NULL;
}

int bfm_bedroom_step_directions(BfmBedroom *room, int confirm_edge, int reveal_edge,
                                int direction, int held, int accept_edge, int back_edge, int horizontal)
{
    return bfm_bedroom_step_axes(room, confirm_edge, reveal_edge, direction, held,
                                 accept_edge, back_edge, horizontal * 127, held * 127, 0);
}

int bfm_bedroom_step_pad(BfmBedroom *room, int confirm_edge, int reveal_edge,
                         int direction, int held, int accept_edge, int back_edge)
{
    return bfm_bedroom_step_directions(room, confirm_edge, reveal_edge, direction,
                                       held, accept_edge, back_edge, 0);
}

int bfm_bedroom_step_controls(BfmBedroom *room,int confirm_edge,int reveal_edge,int direction,int accept_edge)
{
    return bfm_bedroom_step_pad(room,confirm_edge,reveal_edge,direction,direction,accept_edge,
                                confirm_edge && !reveal_edge && !accept_edge);
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
    unsigned crossing = room->village ? bfm_village_arrival_transfer_tick(room->village) : 0;
    if (room->village && room->village_updates >= 18 && !crossing) return 1;
    if (room->village_trip) {
        unsigned tick=room->cable_tick;
        int entering = room->village || (tick >= STATION_START && tick < STATION_START+19);
        /* Both room entries reveal two RGB555 levels on each visible field
         * update. The arriving village first becomes visible at update three. */
        int entry_loss = room->village ? 35 - 2*(int)room->village_updates
                                       : 37 - 2*((int)tick-STATION_START);
        unsigned wipe=tick>=STATION_WIPE ? STATION_WIPE : COURT_WIPE;
        if (crossing || entering || (tick>=wipe && (tick<STATION_START || tick>=STATION_WIPE))) {
            for (unsigned y=0;y<240;++y) {
                unsigned band=(y<120 ? y : 239-y)/8;
                int loss=entering ? entry_loss : (int)((tick-wipe)/2)-(int)band;
                if (crossing) {
                    loss = crossing >= 145 ? 325 - 2*(int)crossing :
                           ((int)crossing-36)/2 - (int)band;
                }
                if (loss<=0) continue;
                for (unsigned x=0;x<320;++x) {
                    uint32_t pix=pixels[y][x], dark=0xff000000u;
                    for (unsigned shift=0;shift<=16;shift+=8) {
                        int component=(int)((pix>>shift)&255)/8-loss;
                        if (component>0) dark|=(unsigned)(component*255/31)<<shift;
                    }
                    pixels[y][x]=dark;
                }
            }
        }
        return 1;
    }
    unsigned wipe_start = room->library ? 43 : 41;
    if (room->transition >= wipe_start || (room->arrival && room->arrival < 23)) {
        int entering = room->arrival != 0;
        int level = entering ? 41 - 2*(int)room->arrival : (int)(room->transition - wipe_start)/2;
        for (unsigned y=0;y<240;++y) {
            int edge=y<120 ? (int)y : 239-(int)y;
            int delay=entering || edge<24 ? 0 : (edge-24)/8;
            if (delay>11) delay=11;
            int shade=level-delay;
            if (shade<=0) continue;
            for (unsigned x=0;x<320;++x) {
                uint32_t p=pixels[y][x], out=0xff000000u;
                for (unsigned shift=0;shift<=16;shift+=8) {
                    int intensity=(int)((p>>shift)&255u)/8-shade;
                    if (intensity>0) out|=(unsigned)(intensity*255/31)<<shift;
                }
                pixels[y][x]=out;
            }
        }
    }
    if (room->message && room->waking) {
        const BfmBedroomRoot *root = bfm_bedroom_travel_root(room->travel);
        int projection[3];
        if (!bfm_bedroom_project(bfm_bedroom_field_view(room->field), root->xyz, projection)) return 0;
        bfm_bedroom_message_art_draw(room->message_art, bfm_forest_message_frame(room->message),
                                     projection[0], pixels);
    }
    return 1;
}

const char *bfm_bedroom_end_message(const BfmBedroom *room)
{
    return room->village_trip ? "Cable ride complete. Start to return." : "Library preview complete. Start to return.";
}

int bfm_bedroom_endpoint(const BfmBedroom *room)
{
    return !room->village_trip && room->message_closed && room->drain==2;
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

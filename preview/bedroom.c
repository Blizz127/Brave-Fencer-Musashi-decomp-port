/* SPDX-License-Identifier: MIT */
#include "forest_maze_wipe.h"
#include "bedroom.h"
#include "inn_field.h"
#include "bakery_field.h"
#include "castle_cable.h"
#include "castle_audience.h"
#include "village_arrival.h"
#include "nightmare_room.h"
#include "nightmare_transition.h"
#include "forest_walk.h"
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
#include "forest_gateway_transition.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
/* The street doorway holds its last image while the shop interior loads.
 * The walk toward the counter starts during the reveal. */
enum { SHOP_FADE_OUT = 16, SHOP_SELECT = 30, INN_CLEAR = 67, INN_WALK = 89,
       INN_FULL_LIGHT = 105, BAKERY_CLEAR = 60, BAKERY_WALK = 81, BAKERY_FULL_LIGHT = 98 };
typedef struct {
    int offset_x, portrait_on_right;
} BedroomWindowLayout;

typedef enum {
    NO_ITEM_ACTION, COLLECT_MOTION, DISPATCH_AWARD, SETUP_AWARD, APPROACH_AWARD, FACE_AWARD, READ_AWARD, LOWER_AWARD, TURN_BACK
} ItemAction;

typedef enum {
    STREET_NORMAL, STREET_REVEALING, STREET_REVEALED
} RecoveredStreet;

typedef struct {
    BfmVillageArrival *village;
    BfmForestHud *hud;
    BfmForestScore *score;
} GrillinExterior;

struct BfmBedroom {
    uint8_t *scene, *main;
    size_t scene_bytes, main_bytes;
    int library, command_room, gesture;
    unsigned command_delay, selected_command;
    BfmCastleAudience *audience;
    uint8_t castle_door[36];
    unsigned castle_tick, castle_reveal;
    int village_court;
    BfmCastleCable *cable;
    BfmVillageArrival *village;
    BfmInnField *inn;
    BfmBakeryField *bakery;
    const uint8_t *shop_door;
    unsigned shop_entry_tick;
    uint16_t bakery_menu_clock;
    BfmVillageImpactSound *landing_sound;
    unsigned village_updates, village_score_wait, inn_arrival;
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
    ForestVoice *voice, *inn_voice, *box_voice;
    int mountain_story_voice;
    const MusashiDiscMedia *disc;
    BfmForestScore *scores[256], *prepared_score, *current_score;
    uint16_t music_field;
    /* A village update can issue one choice sound, every woods request,
     * and one porch knock, in that order. */
    BfmBedroomSound sounds[BFM_VILLAGE_WOODS_SOUNDS + 2];
    size_t sound_count;
    int sounds_taken;
    BfmRetryAudioEvent retry_audio;
    BfmNightmareRoom *nightmare;
    const uint8_t *nightmare_exit;
    int recovery_frame_submitted, mounted_recovery_score, release_room_effects;
    RecoveredStreet recovered_exterior;
    BfmNightmareTransition recovery_transition;
    GrillinExterior pending_exterior;
    uint32_t (*frozen_recovery)[320];
    BfmBedroomEffects *nightmare_effects, *box_effects;
    BfmBedroomEffects *effects, *knock, *shrink_sound, *bakery_effects;
    BfmBedroomEffects *forest_bank_effects, *maze_guide_effects;
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
static void queue_mounted_effect(BfmBedroom *room, BfmBedroomEffects *effects,
                                 BfmRoomSoundBank bank, unsigned command)
{
    if (!command || room->sound_count >= sizeof room->sounds / sizeof *room->sounds) return;
    room->sounds[room->sound_count++] = (BfmBedroomSound){effects, command, bank};
}

static void queue_effect(BfmBedroom *room, BfmRoomSoundBank bank, unsigned command)
{
    BfmBedroomEffects *effects = bank == BFM_ROOM_WOODS_BANK ? room->shrink_sound :
                                 bank == BFM_ROOM_KNOCK_BANK ? room->knock :
                                 bank == BFM_ROOM_BAKERY_BANK ? room->bakery_effects :
                                 bank == BFM_ROOM_BOX_BANK ? room->box_effects :
                                 bank == BFM_ROOM_RECOVERY_BANK ? room->nightmare_effects : room->effects;
    queue_mounted_effect(room,effects,bank,command);
}

static unsigned village_score_selector(const BfmBedroom *room)
{
    unsigned minute = half(room->entry.state.progress + 0x1a);
    return minute >= 420 && minute < 1140 ? 0x0f : 0x10;
}

static BfmForestScore *room_score(BfmBedroom *room, unsigned selector)
{
    if (selector >= sizeof room->scores / sizeof *room->scores) return NULL;
    if (!room->scores[selector])
        room->scores[selector] = bfm_bedroom_score_load(room->disc, selector);
    return room->scores[selector];
}

/* Each receiving room chooses its sequence after its entrance callback. The
 * outgoing gate can fade sooner, while its own field is still displayed. */
static int forest_room_audio(BfmBedroom *room)
{
    if (bfm_village_arrival_forest_score_fades(room->village)) room->current_score = NULL;
    uint16_t field = bfm_village_arrival_room(room->village);
    if (field == room->music_field) return 1;
    const BfmSaveRecord *save = &room->entry.state;
    unsigned story = word(save->progress+8), selector = 0;
    if (field == 0x3014 || field == 0x301b) {
        /* Both fields use the same common and equipped player sounds.
         * Leno's calls occupy the purple field's separate area bank. */
        if (!room->forest_bank_effects)
            room->forest_bank_effects = bfm_forest_bank_effects_load(room->disc);
        if (!room->forest_bank_effects) return 0;
        if (field == 0x301b) {
            if (!room->maze_guide_effects)
                room->maze_guide_effects = bfm_maze_guide_effects_load(room->disc);
            if (!room->maze_guide_effects) return 0;
            selector = story < 750 ? 0x0b : 0x74;
        } else if (story == 1200 && !(save->flags[0x138/8] & (1u << (0x138%8)))) {
            selector = 0x79;
        } else {
            selector = save->progress[0x37] == 4 ? 0x3c : 0x0a;
        }
    } else if (field == 0x1010 && room->music_field == 0x3014) {
        selector = village_score_selector(room);
    }
    if (selector) {
        BfmForestScore *next = room_score(room,selector);
        if (!next) return 0;
        room->current_score = next;
    }
    room->music_field = field;
    return 1;
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
    const BfmSaveRecord *record = &room->entry.state;
    memcpy(run->banks.figure_prices, record->figure_prices, sizeof record->figure_prices);
    memcpy(run->banks.variables, record->variables, sizeof record->variables);
    memcpy(run->banks.timestamps, record->timestamps, sizeof record->timestamps);
    run->player = room->status;
    run->has_player = 1;
    room->run = run;
}

BfmBedroom *bfm_bedroom_open_chapter(const MusashiDiscMedia *disc, const BfmSaveRecord *record,
                                     BfmGameplayRun *run)
{
    if (!run) return NULL;
    BfmBedroom *room = bfm_bedroom_open(disc, record);
    if (room) bfm_bedroom_bind_run(room, run);
    return room;
}

void bfm_bedroom_free(BfmBedroom *room)
{
    if (!room) return;
    for (unsigned i = 0; i < sizeof room->scores / sizeof *room->scores; ++i)
        bfm_forest_score_free(room->scores[i]);
    bfm_forest_message_free(room->message);
    bfm_forest_message_art_free(room->message_art);
    bfm_forest_voice_free(room->voice);
    bfm_forest_voice_free(room->inn_voice);
    bfm_forest_voice_free(room->box_voice);
    bfm_inn_field_free(room->inn);
    bfm_bakery_field_free(room->bakery);
    bfm_nightmare_room_free(room->nightmare);
    bfm_village_arrival_free(room->pending_exterior.village);
    bfm_forest_hud_free(room->pending_exterior.hud);
    free(room->frozen_recovery);
    bfm_castle_audience_free(room->audience);
    bfm_castle_cable_free(room->cable);
    bfm_village_arrival_free(room->village);
    bfm_village_impact_sound_free(room->landing_sound);
    bfm_cable_sound_free(room->slider_sound);
    bfm_forest_hud_free(room->hud);
    bfm_bedroom_effects_free(room->bakery_effects);
    bfm_bedroom_effects_free(room->nightmare_effects);
    bfm_bedroom_effects_free(room->box_effects);
    bfm_bedroom_effects_free(room->knock);
    bfm_bedroom_effects_free(room->shrink_sound);
    bfm_bedroom_effects_free(room->forest_bank_effects);
    bfm_bedroom_effects_free(room->maze_guide_effects);
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
static BfmBedroom *open_entry(const MusashiDiscMedia *disc, const BfmBedroomEntry *entry)
{
    BfmBedroom *room = calloc(1, sizeof *room);
    if (!room) return NULL;
    room->disc = disc;
    room->entry = *entry;
    room->story_flags = room->entry.state.flags;
    room->run = &room->local_run;
    if (entry->scene_member == 1) {
        room->main = bfm_bedroom_player_data(disc, &room->main_bytes);
        if (!room->main) goto failed;
        room->village = bfm_village_resume_inn(disc, &room->entry, room->main,
                                room->main_bytes, room->tiles, &room->hud);
        if (!room->village) goto failed;
        room->voice = bfm_bedroom_voice_load(disc);
        room->current_score = room_score(room, village_score_selector(room));
        if (!room->current_score) goto failed;
        room->effects = bfm_menu_effects_load(disc);
        if (!room->voice || !room->effects) goto failed;
        memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
        bfm_forest_hud_show(room->hud);
        room->village_trip = 1;
        return room;
    }
    room->waking = 1;
    room->scene = malloc(BFM_FIELD_SCENE_CAPACITY);
    if (!room->scene) goto failed;
    room->scene_bytes = bfm_read_field_scene(disc, entry->scene_member, room->scene);
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
        !bfm_scene_read_tiles(disc, "SC01.CD", entry->scene_member, room->tiles) ||
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

BfmBedroom *bfm_bedroom_open(const MusashiDiscMedia *disc, const BfmSaveRecord *record)
{
    BfmBedroomEntry entry;
    return bfm_bedroom_entry(disc, record, &entry) ? open_entry(disc, &entry) : NULL;
}

BfmBedroom *bfm_bedroom_open_restart(const MusashiDiscMedia *disc, const BfmSaveRecord *record)
{
    BfmBedroomEntry entry;
    if (!bfm_bedroom_entry(disc, record, &entry) || entry.scene_member != 6 ||
        word(record->destination) != word(entry.state.destination)) return NULL;
    memcpy(entry.state.destination, record->destination, sizeof entry.state.destination);
    return open_entry(disc, &entry);
}

static void clear_castle_field(BfmBedroom *r)
{
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
}

static int load_castle_room(BfmBedroom *r, const uint8_t *destination, int court)
{
    BfmBedroomEntry entry = r->entry;
    memcpy(entry.state.flags, r->story_flags, sizeof entry.state.flags);
    if (!bfm_castle_room_entry(r->disc, destination, &entry)) return 0;
    unsigned entrance = entry.state.destination[2];
    clear_castle_field(r);
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
    if (!r->village_court) entry.state.flags[303 / 8] &= (uint8_t)~(1u << (303 % 8));
    memcpy(r->story_flags, entry.state.flags, sizeof entry.state.flags);
    r->entry=entry;
    r->library=!court;
    r->command_room=court;
    r->entry_wait=4;
    r->arrival=1;
    return 1;
}

static int enter_castle_room(BfmBedroom *room)
{
    int court = room->library;
    const uint8_t *door = data_range(room, court ? 0x8018317cu : 0x80185e5cu,
                                      sizeof room->entry.state.destination);
    return load_castle_room(room, door, court);
}

/* The street, courtyard and Visit audience own different field data. Build
 * the returning courtyard and its tile pages before relinquishing the old room. */
static int receive_courtyard(BfmBedroom *room, const uint8_t *door)
{
    BfmForestScore *score = room_score(room, 0x3d);
    if (!score) return 0;
    BfmBedroom *next = calloc(1, sizeof *next);
    if (!next) return 0;
    next->disc = room->disc;
    next->entry = room->entry;
    memcpy(next->entry.state.flags, room->story_flags, sizeof next->entry.state.flags);
    next->story_flags = next->entry.state.flags;
    next->main = room->main;
    next->main_bytes = room->main_bytes;
    next->village_court = 1;
    memcpy(next->tiles, room->tiles, sizeof next->tiles);
    next->scene = malloc(BFM_FIELD_SCENE_CAPACITY);
    if (!next->scene || !bfm_hero_run_smoke_load(&next->dust.sprite,next->main,next->main_bytes) ||
        !load_castle_room(next, door, 1)) {
        next->main = NULL;
        bfm_bedroom_free(next);
        return 0;
    }
    clear_castle_field(room);
    free(room->scene);
    room->scene = next->scene;
    room->scene_bytes = next->scene_bytes;
    room->field = next->field;
    room->travel = next->travel;
    room->motion = next->motion;
    room->figure = next->figure;
    room->visitors = next->visitors;
    room->message_art = next->message_art;
    room->blade = next->blade;
    room->dust = next->dust;
    room->script = next->script;
    room->pose = next->pose;
    room->entry = next->entry;
    room->command_delay = next->command_delay;
    room->entry_wait = next->entry_wait;
    room->arrival = next->arrival;
    memcpy(room->camera_actor, next->camera_actor, sizeof room->camera_actor);
    memcpy(room->tiles, next->tiles, sizeof room->tiles);
    memcpy(room->story_flags, room->entry.state.flags, sizeof room->entry.state.flags);
    memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
    free(next);
    bfm_castle_audience_free(room->audience); room->audience = NULL;
    bfm_village_arrival_free(room->village); room->village = NULL;
    bfm_castle_cable_free(room->cable); room->cable = NULL;
    bfm_forest_message_free(room->message); room->message = NULL;
    room->village_trip = room->cable_tick = 0;
    room->recovered_exterior = STREET_NORMAL;
    room->books_active = room->message_closed = room->drain = 0;
    room->running = room->waking = room->gesture = 0;
    room->requested = room->pending = BFM_BEDROOM_NONE;
    room->current_score = score;
    room->command_room = room->village_court = 1;
    room->library = 0;
    bfm_bedroom_hud_hide(room->hud);
    return 1;
}

static void begin_castle_transfer(BfmBedroom *room, const uint8_t *door)
{
    memcpy(room->castle_door, door, sizeof room->castle_door);
    room->castle_tick = 1;
    room->castle_reveal = 0;
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
        room->prepared_score = room_score(room, operand);
        if (!room->prepared_score) return 0;
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
    int first_village = !room->village_court && room->selected_command == 4 && !address &&
                        !(room->story_flags[181 / 8] & (1u << (181 % 8)));
    if (room->selected_command == 4 && !address)
        address = first_village ? 0x80182c74u : 0x80182d24u;
    if (address) {
        const uint8_t *door=data_range(room,address,sizeof room->entry.state.destination);
        if (!door) return 0;
        if (room->village_court &&
            ((room->selected_command == 0 && word(door) == 0x3003 &&
              bfm_castle_audience_eligible(&room->entry.state)) ||
             (room->selected_command == 4 && word(door) == 0x00031010))) {
            memcpy(room->frames[room->presented], room->books_background, sizeof room->frames[0]);
            bfm_castle_hud_command_closed(room->hud);
            begin_castle_transfer(room, door);
            return 1;
        }
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

static ForestVoice *current_voice(const BfmBedroom *room)
{
    if (room->inn || room->bakery) return room->inn_voice;
    if (!room->mountain_story_voice && room->village && bfm_village_arrival_room(room->village) == 0x3026)
        return room->box_voice;
    return room->voice;
}

static void advance_voice(BfmBedroom *room)
{
    ForestVoice *voice = current_voice(room);
    unsigned old_status = bfm_forest_voice_status(voice);
    bfm_forest_voice_step(voice);
    if (old_status == BFM_FOREST_VOICE_PENDING &&
        bfm_forest_voice_status(voice) == BFM_FOREST_VOICE_PLAYING)
        room->voice_ready_field = 2 * room->update;
}

static int request_field_voice(BfmBedroom *room, unsigned bank, unsigned cue)
{
    ForestVoice *prior = current_voice(room), *source = prior;
    /* ST02 and OUT share the mountain field. Keep the selected decoder
     * active across text pages and independent actor calls. */
    int mountain = room->village && bfm_village_arrival_room(room->village) == 0x3026;
    if (mountain && bank == 1) source = room->voice;
    if (mountain && bank == 11) source = room->box_voice;
    if (!source || !bfm_forest_voice_request(source, bank, cue)) return 0;
    if (prior && source != prior && bfm_forest_voice_status(prior) != BFM_FOREST_VOICE_IDLE)
        bfm_forest_voice_stop(prior);
    if (mountain) room->mountain_story_voice = source == room->voice;
    ++room->generation;
    return 1;
}

static void apply_window_voice(BfmBedroom *room, const BfmForestMessageFrame *frame, int *result)
{
    if (!frame) return;
    ForestVoice *voice = current_voice(room);
    if (frame->events & BFM_FOREST_VOICE_STOP) {
        bfm_forest_voice_stop(voice);
        *result = BFM_BEDROOM_VOICE_STOPPED;
    }
    if (frame->events & BFM_FOREST_VOICE_REQUEST)
        request_field_voice(room, frame->voice_bank, frame->voice_cue);
}

static const BfmForestMessageFrame *advance_message(BfmBedroom *room, int confirm_edge,
                                                     int reveal_edge, int *result)
{
    const BfmForestMessageFrame *frame = bfm_field_message_tick(room->message,
                                bfm_forest_voice_status(current_voice(room)), confirm_edge, reveal_edge);
    apply_window_voice(room, frame, result);
    return frame;
}

static BfmRetryScreen *current_field_retry(const BfmBedroom *room)
{
    return room ? bfm_village_arrival_retry(room->village) : NULL;
}

static int submit_village_frame(BfmBedroom *room)
{
    if (bfm_village_arrival_maze_crossing(room->village)) {
        unsigned tick = bfm_village_arrival_transfer_tick(room->village);
        if (tick > BFM_MAZE_LAST_SOURCE_TICK && tick < BFM_MAZE_REVEAL_TICK) {
            /* The outdoor shutter keeps a copy of the last submitted source
             * screen while the next room is loading. */
            ++room->village_updates;
            ++room->update;
            return 1;
        }
    }
    int detector = bfm_village_arrival_detector(room->village);
    bfm_forest_hud_detector(room->hud, detector >= 0, detector < 0 ? 0u : (unsigned)detector);
    int hud_request = bfm_village_arrival_take_hud(room->village);
    if (hud_request < 0) bfm_bedroom_hud_hide(room->hud);
    if (hud_request > 0) bfm_forest_hud_show(room->hud);
    bfm_forest_hud_tick(room->hud);
    if (!bfm_village_arrival_rest_hud(room->village, room->hud)) return 0;
    uint32_t (*frame)[320] = room->frames[room->presented];
    memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
    bfm_forest_hud_submit_points(room->hud, &room->status);
    uint16_t field_id = bfm_village_arrival_room(room->village);
    if (field_id == 0x3025 || field_id == 0x3014 || field_id == 0x301b) {
        bfm_woods_hud_gauges(room->hud, room->status.record[0x47],
                            bfm_village_arrival_tether_gauge(room->village));
    }
    if (current_field_retry(room)) {
        if (!bfm_village_arrival_draw_retry(room->village, room->hud, frame)) return 0;
    } else {
        const BfmForestMessageFrame *window = room->message ? bfm_forest_message_frame(room->message) : NULL;
        if (!bfm_village_arrival_draw(room->village, room->story_flags, window, frame)) return 0;
        bfm_forest_hud_draw(room->hud, &room->status, frame);
        bfm_village_arrival_submit_woods_crossing(room->village, frame,
                                                room->frames[(room->presented + 1) % 3]);
    }
    room->presented = (room->presented + 1) % 3;
    ++room->village_updates;
    ++room->update;
    return 1;
}

static int queue_woods_sounds(BfmBedroom *room)
{
    unsigned commands[BFM_VILLAGE_WOODS_SOUNDS];
    size_t count = bfm_village_arrival_take_woods_sounds(room->village, commands, BFM_VILLAGE_WOODS_SOUNDS);
    if (!count) return 1;
    if (room->music_field == 0x3014 || room->music_field == 0x301b) {
        for (size_t i = 0; i < count; ++i) {
            unsigned cue = commands[i];
            int guide = room->music_field == 0x301b &&
                (cue == BFM_WOODS_LENO_BARK_SOUND || cue == BFM_MAZE_GUIDE_SNIFF_SOUND ||
                 cue == BFM_MAZE_GUIDE_ABSENT_SOUND);
            queue_mounted_effect(room,guide ? room->maze_guide_effects : room->forest_bank_effects,
                                 guide ? BFM_ROOM_GUIDE_BANK : BFM_ROOM_WOODS_BANK,cue);
        }
        return 1;
    }
    if (!room->shrink_sound) room->shrink_sound = bfm_woods_shrink_sound_load(room->disc);
    if (!room->shrink_sound) return 0;
    for (size_t i = 0; i < count; ++i)
        queue_effect(room, BFM_ROOM_WOODS_BANK, commands[i]);
    return 1;
}

/* The outgoing room owns its actor and voice until the confirmed transfer
 * has completed and the incoming room has produced its first drawable pose. */
static int mount_grillin_exterior(BfmBedroom *room, const uint8_t *door, GrillinExterior *out);
static void install_grillin_exterior(BfmBedroom *room, GrillinExterior *out, int from_inn);

/* Other fields can record temporary checkpoints before their native receiver exists. */
static int unmounted_box_endpoint(const BfmBedroom *room)
{
    return bfm_bedroom_retry_destination(room) == BFM_WOODS_RESTORE_BOX &&
           (!room->run || word(room->run->memory_box.state.destination) != 0x00ff3026u);
}

static int open_woods_recovery(BfmBedroom *room)
{
    BfmBedroomEntry restored;
    if (!bfm_bedroom_prepare_woods_recovery(room, &restored)) return 0;
    if (bfm_bedroom_retry_destination(room) == BFM_WOODS_RESTORE_BOX) {
        BfmForestScore *score = room_score(room, 0x18);
        if (!score || !bfm_village_arrival_restore_woods_box(room->village, &restored)) return 0;
        room->current_score = score;
        room->story_flags = room->entry.state.flags;
        memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
        bfm_bedroom_bind_run(room, room->run);
        room->recovery_frame_submitted = 0;
        room->recovered_exterior = STREET_NORMAL;
        bfm_forest_hud_show(room->hud);
        return submit_village_frame(room) ? BFM_BEDROOM_VOICE_STOPPED : 0;
    }
    if (!room->mounted_recovery_score) {
        BfmForestScore *score = room_score(room, 0x87);
        if (!score) return 0;
        /* The loader admits the score before the receiving player's first update. */
        room->current_score = score;
        room->mounted_recovery_score = 1;
        return BFM_BEDROOM_ADVANCED;
    }
    if (!room->nightmare_effects) room->nightmare_effects = bfm_nightmare_sound_load(room->disc);
    if (!room->nightmare_effects ||
        !bfm_nightmare_transition_open(room->disc, &room->recovery_transition)) return 0;
    BfmNightmareRoom *incoming = bfm_nightmare_room_open(room->disc, &restored);
    if (!incoming) return 0;
    unsigned first = (room->presented + 1) % 3;
    if (!bfm_nightmare_room_draw(incoming, room->frames[first])) {
        bfm_nightmare_room_free(incoming);
        return 0;
    }

    bfm_forest_message_free(room->message);
    room->message = NULL;
    bfm_village_arrival_free(room->village);
    room->village = NULL;
    room->entry = restored;
    room->story_flags = room->entry.state.flags;
    memcpy(room->status.record, restored.state.progress, sizeof room->status.record);
    bfm_bedroom_bind_run(room, room->run);
    bfm_nightmare_transition_begin_arrival(&room->recovery_transition);
    room->mounted_recovery_score = 0;
    room->nightmare = incoming;
    room->recovery_frame_submitted = 0;
    room->presented = first;
    return BFM_BEDROOM_VOICE_STOPPED;
}

static int advance_recovery_exit(BfmBedroom *room)
{
    ForestVoice *voice = bfm_nightmare_room_voice(room->nightmare);
    int idle = bfm_forest_voice_status(voice) == BFM_FOREST_VOICE_IDLE;
    BfmNightmareTransitionEvent event = bfm_nightmare_transition_step(&room->recovery_transition,
        idle, room->frozen_recovery != NULL, room->pending_exterior.village != NULL);
    int result = BFM_BEDROOM_ADVANCED;
    switch (event) {
    case BFM_NIGHTMARE_STOP_OPTICAL:
        bfm_forest_voice_stop(voice);
        result = BFM_BEDROOM_VOICE_STOPPED;
        break;
    case BFM_NIGHTMARE_PREPARE_VILLAGE:
        if (!mount_grillin_exterior(room, room->nightmare_exit, &room->pending_exterior)) return 0;
        break;
    case BFM_NIGHTMARE_EXIT_CAMERA:
        if (!bfm_nightmare_room_exit_camera(room->nightmare)) return 0;
        break;
    case BFM_NIGHTMARE_RELEASE_ROOM_AUDIO:
        room->release_room_effects = 1;
        room->current_score = NULL;
        break;
    case BFM_NIGHTMARE_CAPTURE_OUTGOING:
        room->frozen_recovery = malloc(sizeof room->frames[0]);
        if (!room->frozen_recovery) return 0;
        memcpy(room->frozen_recovery, room->frames[room->presented], sizeof room->frames[0]);
        break;
    case BFM_NIGHTMARE_ENTER_VILLAGE:
        install_grillin_exterior(room, &room->pending_exterior, 0);
        return BFM_BEDROOM_VOICE_STOPPED;
    default:
        break;
    }
    unsigned next = (room->presented + 1) % 3;
    if (room->frozen_recovery)
        memcpy(room->frames[next], room->frozen_recovery, sizeof room->frames[0]);
    else if (!bfm_nightmare_room_draw(room->nightmare, room->frames[next]))
        return 0;
    room->presented = next;
    ++room->update;
    return result;
}

static int nightmare_step(BfmBedroom *room, int confirm, int reveal)
{
    if (room->nightmare_exit) return advance_recovery_exit(room);
    BfmNightmareRoomEvent event;
    unsigned before = bfm_forest_voice_status(bfm_nightmare_room_voice(room->nightmare));
    BfmNightmareStep step = bfm_nightmare_room_step_field(room->nightmare, confirm, reveal, 0, &event);
    if (step == BFM_NIGHTMARE_ERROR) return 0;
    if (before == BFM_FOREST_VOICE_PENDING &&
        bfm_forest_voice_status(bfm_nightmare_room_voice(room->nightmare)) == BFM_FOREST_VOICE_PLAYING)
        room->voice_ready_field = 2 * room->update;
    queue_effect(room, BFM_ROOM_RECOVERY_BANK, event.cue);
    unsigned next = (room->presented + 1) % 3;
    if (!bfm_nightmare_room_draw(room->nightmare, room->frames[next])) return 0;
    room->presented = next;
    room->nightmare_exit = event.destination;
    if (room->nightmare_exit)
        bfm_nightmare_transition_begin_departure(&room->recovery_transition);
    else
        bfm_nightmare_transition_step(&room->recovery_transition, 1, 0, 0);
    ++room->update;
    return event.voice_stopped ? BFM_BEDROOM_VOICE_STOPPED : BFM_BEDROOM_ADVANCED;
}

static int woods_retry_step(BfmBedroom *room, BfmRetryScreen *retry, BfmFieldInput field,
                           int direction, int cancel, int accept, int reveal)
{
    if (room->recovery_frame_submitted) return open_woods_recovery(room);
    unsigned pressed = (field.square_pressed ? BFM_FOREST_SQUARE : 0u) |
                       (field.triangle_pressed || cancel ? BFM_FOREST_TRIANGLE : 0u) |
                       (field.circle_pressed || reveal ? BFM_FOREST_CIRCLE : 0u) |
                       (field.cross_pressed || accept ? BFM_FOREST_CROSS : 0u);
    unsigned held = field.held_faces | pressed | (field.cross_held ? BFM_FOREST_CROSS : 0u);
    ForestVoice *voice = bfm_retry_screen_voice(retry);
    unsigned before = bfm_forest_voice_status(voice);
    if (bfm_retry_screen_choose_step(retry, held, pressed, direction, cancel || reveal, accept) < 0)
        return 0;
    if (before == BFM_FOREST_VOICE_PENDING && bfm_forest_voice_status(voice) == BFM_FOREST_VOICE_PLAYING)
        room->voice_ready_field = 2 * room->update;
    if (bfm_retry_screen_choice_changed(retry))
        queue_effect(room, BFM_ROOM_SCENE_BANK, accept ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND);
    room->retry_audio = bfm_retry_screen_audio_event(retry);
    if (room->retry_audio == BFM_RETRY_AUDIO_START) room->current_score = NULL;
    if (!submit_village_frame(room)) return 0;
    /* The saturating wipe callback submits the outgoing field before the
     * common room sequence hands the framebuffer to the next scene. */
    BfmWoodsRetryPage destination = bfm_retry_screen_woods_destination(retry);
    if (destination == BFM_WOODS_RESTORE_SAVED ||
        (destination == BFM_WOODS_RESTORE_BOX && !unmounted_box_endpoint(room)))
        room->recovery_frame_submitted = 1;
    return bfm_retry_screen_voice_stopped(retry) ? BFM_BEDROOM_VOICE_STOPPED : BFM_BEDROOM_ADVANCED;
}

static int village_step(BfmBedroom *room, int confirm_edge, int reveal_edge, int direction,
                        int accept_edge, BfmFieldInput field)
{
    int result = BFM_BEDROOM_ADVANCED;
    const uint8_t *arch = bfm_village_arrival_castle_transfer(room->village);
    if (arch && !room->message && bfm_castle_audience_eligible(&room->entry.state)) {
        begin_castle_transfer(room, arch);
        return BFM_BEDROOM_ADVANCED;
    }
    if (bfm_village_arrival_room(room->village) == 0x3026 && !room->box_voice) {
        room->box_voice = bfm_defeat_voice_load(room->disc);
        if (!room->box_voice) return 0;
    }
    int field_notice = bfm_field_message_keeps_control(room->message);
    int was_in_dialogue = room->message && !field_notice;
    advance_voice(room);
    if (room->message) {
        if (bfm_field_message_choose(room->message, direction, confirm_edge && !accept_edge, accept_edge))
            queue_effect(room, BFM_ROOM_SCENE_BANK, accept_edge ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND);
        const BfmForestMessageFrame *frame = advance_message(room, confirm_edge, reveal_edge, &result);
        if (!frame) return 0;
        if ((frame->actor_flags & 1u) && bfm_village_arrival_room(room->village) == 0x3026) {
            /* The challenger consumes the speaker's gesture on this field's actor pass. */
            room->entry.state.flags[0x125 / 8] |= (uint8_t)(1u << (0x125 % 8));
        }
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
            unsigned answer = frame->choices ? frame->answer : 0;
            bfm_forest_message_free(room->message);
            room->message = NULL;
            if (!field_notice && !bfm_village_arrival_message_closed(room->village, answer)) return 0;
        }
    }
    if (room->village_score_wait && --room->village_score_wait == 0) {
        if (!apply(room, BFM_BED_SOUND_READY, 0) || !bfm_village_arrival_continue(room->village))
            return 0;
    }
    BfmForestMessage *opened = NULL;
    if (was_in_dialogue) {
        field.select_pressed = 0;
        field.square_pressed = field.triangle_pressed = field.shoulder_held = 0;
        field.circle_pressed = field.cross_pressed = field.cross_held = 0;
    }
    int previous_score = bfm_village_arrival_field_score(room->village, village_score_selector(room));
    if (!bfm_village_arrival_step_field(room->village, room->run, field, &opened)) return 0;
    if (bfm_village_arrival_take_retired_window(room->village)) {
        bfm_forest_message_free(room->message);
        room->message = NULL;
    }
    unsigned field_voice = bfm_village_arrival_take_box_voice(room->village);
    if (field_voice && !request_field_voice(room, 1, field_voice)) return 0;
    int next_score = bfm_village_arrival_field_score(room->village, village_score_selector(room));
    if (previous_score != next_score && next_score < 0) {
        room->current_score = NULL;
    } else if (previous_score != next_score) {
        unsigned selector = next_score ? (unsigned)next_score : village_score_selector(room);
        BfmForestScore *score = room_score(room, selector);
        if (!score) return 0;
        room->current_score = score;
        bfm_village_arrival_woods_score_applied(room->village, selector);
    }
    if (opened) {
        if (field_notice) bfm_forest_message_free(room->message);
        room->message = opened;
    }
    if (!forest_room_audio(room) || !queue_woods_sounds(room)) return 0;
    unsigned box_cue;
    while ((box_cue = bfm_village_arrival_take_box_sound(room->village)) != 0) {
        if (!room->box_effects) room->box_effects = bfm_woods_box_sound_load(room->disc);
        if (!room->box_effects) return 0;
        queue_effect(room, BFM_ROOM_BOX_BANK, box_cue);
    }
    if (!bfm_village_arrival_begin_retry(room->village, room->run)) return 0;
    if (current_field_retry(room)) {
        bfm_forest_message_free(room->message);
        room->message = NULL;
        ForestVoice *voice = current_voice(room);
        if (bfm_forest_voice_status(voice) != BFM_FOREST_VOICE_IDLE) {
            bfm_forest_voice_stop(voice);
            result = BFM_BEDROOM_VOICE_STOPPED;
        }
        return submit_village_frame(room) ? result : 0;
    }
    const uint8_t *shop_door = bfm_village_arrival_shop_transfer(room->village);
    if (shop_door) {
        room->shop_door = shop_door;
        room->shop_entry_tick = 0;
        room->current_score = NULL;
        return submit_village_frame(room) ? BFM_BEDROOM_ADVANCED : 0;
    }
    if (room->story_flags != room->entry.state.flags)
        memcpy(room->story_flags, room->entry.state.flags, sizeof room->entry.state.flags);
    const BfmVillagePlayer *player = bfm_village_arrival_player(room->village);
    unsigned upper_sound = bfm_village_arrival_take_upper_sound(room->village);
    if (upper_sound || player->pose.knock_contact || (player->events & BFM_VILLAGE_WELL_SOUND_EVENT)) {
        if (!room->knock) room->knock = bfm_village_knock_sound_load(room->disc);
        if (!room->knock) return 0;
        if (player->pose.knock_contact)
            queue_effect(room, BFM_ROOM_KNOCK_BANK, BFM_VILLAGE_KNOCK_SOUND);
        if (player->events & BFM_VILLAGE_WELL_SOUND_EVENT)
            queue_effect(room, BFM_ROOM_KNOCK_BANK, BFM_VILLAGE_WELL_SOUND);
        if (upper_sound) queue_effect(room, BFM_ROOM_KNOCK_BANK, upper_sound);
    }
    if (player->events & BFM_VILLAGE_CAMERA) {
        if (!apply(room, BFM_BED_SOUND_CHANGE, village_score_selector(room)))
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
    return submit_village_frame(room) ? result : 0;
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

const BfmSaveRecord *bfm_bedroom_inn_record(const BfmBedroom *room)
{
    return room->inn && bfm_inn_field_service(room->inn) ? &room->entry.state : NULL;
}

BfmInnField *bfm_bedroom_inn_counter(BfmBedroom *room)
{
    return bfm_inn_field_service(room->inn) ? room->inn : NULL;
}

int bfm_bedroom_leave_inn(BfmBedroom *room)
{
    return bfm_inn_field_leave(room->inn);
}

static int mount_grillin_exterior(BfmBedroom *room, const uint8_t *door, GrillinExterior *out)
{
    BfmBedroomEntry previous = room->entry, incoming = previous;
    if (!bfm_castle_room_entry(room->disc, door, &incoming)) return 0;
    BfmForestScore *score = room_score(room, village_score_selector(room));
    if (!score) return 0;

    /* The inn counter shares the street's tile allocation. Restore it if
     * street setup fails while the counter still owns the display. */
    void *inn_tiles = NULL;
    if (room->inn || room->bakery || room->village_court) {
        inn_tiles = malloc(sizeof room->tiles);
        if (!inn_tiles) return 0;
        memcpy(inn_tiles, room->tiles, sizeof room->tiles);
    }
    room->entry = incoming;
    BfmForestHud *hud = NULL;
    BfmVillageArrival *outside = bfm_village_resume_inn(room->disc, &room->entry, room->main,
                                                       room->main_bytes, room->tiles, &hud);
    if (!outside) {
        room->entry = previous;
        if (inn_tiles) memcpy(room->tiles, inn_tiles, sizeof room->tiles);
    }
    free(inn_tiles);
    if (!outside) return 0;
    *out = (GrillinExterior){.village = outside, .hud = hud, .score = score};
    return 1;
}

static void install_grillin_exterior(BfmBedroom *room, GrillinExterior *out, int from_inn)
{
    bfm_village_arrival_free(room->village);
    bfm_nightmare_room_free(room->nightmare);
    free(room->frozen_recovery);
    room->nightmare = NULL;
    room->nightmare_exit = NULL;
    room->frozen_recovery = NULL;
    bfm_inn_field_free(room->inn);
    bfm_bakery_field_free(room->bakery);
    bfm_forest_hud_free(room->hud);
    room->inn = NULL;
    room->bakery = NULL;
    room->village = out->village;
    room->hud = out->hud;
    bfm_village_arrival_set_scene_tick(room->village, room->update);
    room->current_score = out->score;
    *out = (GrillinExterior){0};
    room->inn_arrival = from_inn;
    room->recovered_exterior = from_inn ? STREET_NORMAL : STREET_REVEALING;
    if (!from_inn) {
        bfm_village_arrival_begin_hotelo_reveal(room->village);
        bfm_nightmare_transition_begin_arrival(&room->recovery_transition);
    }
    bfm_forest_hud_show(room->hud);
    room->village_updates = 0;
    room->presented = 0;
    memset(room->frames, 0, sizeof room->frames);
}

static int return_to_street(BfmBedroom *room, const uint8_t *door)
{
    GrillinExterior outside;
    if (!mount_grillin_exterior(room, door, &outside)) return 0;
    install_grillin_exterior(room, &outside, 1);
    return 1;
}

/* The village's long room change first shows the new courtyard 168 fields
 * after the arch. The two indoor castle transitions use a shorter load. */
static unsigned castle_load_fields(const BfmBedroom *room)
{
    if (room->village) return 168;
    return room->audience ? 127 : 129;
}

static int transfer_castle_field(BfmBedroom *room)
{
    unsigned target = half(room->castle_door);
    ++room->update;
    if (++room->castle_tick <= castle_load_fields(room)) {
        if (room->village && room->castle_tick == 41) room->current_score = NULL;
        return 1;
    }
    if (target == 0x3000) {
        if (!receive_courtyard(room, room->castle_door)) return 0;
    } else if (target == 0x3003) {
        BfmBedroomEntry entry = room->entry;
        memcpy(entry.state.flags, room->story_flags, sizeof entry.state.flags);
        if (!bfm_castle_audience_eligible(&entry.state) ||
            !bfm_castle_room_entry(room->disc, room->castle_door, &entry)) return 0;
        BfmCastleAudience *audience = bfm_castle_audience_open(room->disc, &entry,
                                                               room->main, room->main_bytes);
        if (!audience) return 0;
        room->audience = audience;
        room->entry = entry;
        room->castle_reveal = 17;
        room->message_closed = room->drain = 0;
        bfm_bedroom_hud_hide(room->hud);
    } else if (target == 0x1010) {
        if (!room->voice) room->voice = bfm_bedroom_voice_load(room->disc);
        if (!room->voice || !return_to_street(room, room->castle_door)) return 0;
        room->command_room = room->village_court = 0;
        room->message_closed = room->drain = 0;
        room->village_trip = 1;
    } else return 0;
    room->castle_tick = 0;
    return 1;
}

int bfm_bedroom_step_field(BfmBedroom *room, int confirm_edge, int reveal_edge,
                           int direction, int held, int accept_edge, int back_edge, BfmFieldInput field)
{
    room->sound_count = 0;
    room->sounds_taken = 0;
    room->slider_event = 0;
    room->retry_audio = BFM_RETRY_AUDIO_NONE;
    if (room->nightmare) {
        if (bfm_nightmare_transition_live(&room->recovery_transition))
            bfm_gameplay_advance_playtime(room->entry.state.progress, 1);
        return nightmare_step(room, accept_edge, reveal_edge);
    }
    BfmRetryScreen *retry = current_field_retry(room);
    if (retry) return woods_retry_step(room, retry, field, direction, back_edge, accept_edge, reveal_edge);
    int result = BFM_BEDROOM_ADVANCED;
    if (room->castle_tick) return transfer_castle_field(room);
    if (room->castle_reveal) {
        ++room->update;
        --room->castle_reveal;
        return BFM_BEDROOM_ADVANCED;
    }
    if (room->audience) {
        bfm_gameplay_advance_playtime(room->entry.state.progress, 1);
        result = bfm_castle_audience_step(room->audience, &room->entry.state, room->run,
                                          confirm_edge, reveal_edge, direction, held, accept_edge, back_edge);
        if (!result) return 0;
        queue_mounted_effect(room, bfm_castle_audience_effects(room->audience), BFM_ROOM_SCENE_BANK,
                             bfm_castle_audience_take_sound(room->audience));
        memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
        memcpy(room->story_flags, room->entry.state.flags, sizeof room->entry.state.flags);
        const uint8_t *door = bfm_castle_audience_transfer(room->audience);
        if (door) {
            if (word(door) != 0x00043000 || !bfm_castle_audience_draw(room->audience, room->hud,
                                             &room->status, room->frames[room->presented])) return 0;
            begin_castle_transfer(room, door);
        }
        ++room->update;
        return result;
    }
    if (room->recovered_exterior != STREET_REVEALING &&
        !bfm_village_arrival_woods_playtime_paused(room->village))
        bfm_gameplay_advance_playtime(room->entry.state.progress, 1);
    if (room->shop_door) {
        unsigned tick = ++room->shop_entry_tick;
        int bakery = half(room->shop_door) == 0x2015;
        unsigned walk = bakery ? BAKERY_WALK : INN_WALK;
        unsigned full_light = bakery ? BAKERY_FULL_LIGHT : INN_FULL_LIGHT;
        if (tick == 1) bfm_village_arrival_lower_ambient(room->village, room->run);
        if (tick <= 3) return submit_village_frame(room) ? BFM_BEDROOM_ADVANCED : 0;
        if (tick == SHOP_SELECT) {
            if (!room->inn_voice) room->inn_voice = bfm_inn_voice_load(room->disc);
            if (!room->inn_voice) return 0;
            if (bakery) {
                if (!room->bakery_effects) room->bakery_effects = bfm_bakery_effects_load(room->disc);
                if (!room->bakery_effects) return 0;
                room->bakery = bfm_bakery_field_open(room->disc, room->shop_door, &room->entry.state,
                                                     room->main, room->main_bytes, room->tiles,
                                                     &room->bakery_menu_clock);
                if (!room->bakery) return 0;
            } else {
                room->inn = bfm_inn_field_open(room->disc, room->shop_door, &room->entry.state, room->tiles);
                if (!room->inn) return 0;
            }
            memcpy(room->entry.state.destination, room->shop_door, sizeof room->entry.state.destination);
        }
        /* The held street frame keeps its HUD. The incoming status panels
         * slide back into position as Musashi starts walking to the counter. */
        if (tick >= SHOP_SELECT) {
            if (tick < walk) bfm_bedroom_hud_hide(room->hud);
            if (tick == walk) bfm_forest_hud_show(room->hud);
            bfm_forest_hud_tick(room->hud);
        }
        if (tick < walk) {
            ++room->update;
            return BFM_BEDROOM_ADVANCED;
        }
        if (tick >= full_light) room->shop_door = NULL;
    }
    if (room->inn || room->bakery) {
        uint16_t pressed = (direction < 0 ? 0x10u : direction > 0 ? 0x40u : 0u) |
                           (back_edge ? 0x1000u : 0u) | (reveal_edge ? 0x2000u : 0u) |
                           (accept_edge ? 0x4000u : 0u);
        advance_voice(room);
        const uint8_t *outside;
        unsigned voice_status = bfm_forest_voice_status(room->inn_voice);
        if (room->bakery) {
            pressed |= field.pressed_directions;
            uint16_t directions = (field.y < 0 ? 0x10u : field.y > 0 ? 0x40u : 0u) |
                                  (field.x < 0 ? 0x80u : field.x > 0 ? 0x20u : 0u);
            if (!bfm_bakery_field_step(room->bakery, pressed, directions, voice_status, room->run)) return 0;
            queue_effect(room, BFM_ROOM_BAKERY_BANK, bfm_bakery_field_sound(room->bakery));
            apply_window_voice(room, bfm_bakery_field_window(room->bakery), &result);
            outside = bfm_bakery_field_departure(room->bakery);
        } else {
            bfm_inn_field_voice_status(room->inn, voice_status);
            if (!bfm_inn_field_step_run(room->inn, pressed, room->run)) return 0;
            apply_window_voice(room, bfm_inn_field_window(room->inn), &result);
            outside = bfm_inn_field_departure(room->inn);
        }
        if (outside && !return_to_street(room, outside)) return 0;
        if (room->story_flags != room->entry.state.flags)
            memcpy(room->story_flags, room->entry.state.flags, sizeof room->entry.state.flags);
        memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
        bfm_forest_hud_submit_points(room->hud, &room->status);
        ++room->update;
        return result;
    }
    if (room->recovered_exterior == STREET_REVEALING) {
        BfmNightmareTransitionEvent event = bfm_nightmare_transition_step(&room->recovery_transition, 1, 0, 0);
        if (event == BFM_NIGHTMARE_REVEAL_COMPLETE) room->recovered_exterior = STREET_REVEALED;
        return village_step(room, 0, 0, 0, 0, (BfmFieldInput){0});
    }
    if (room->recovered_exterior == STREET_REVEALED && room->village_updates >= 18)
        room->recovered_exterior = STREET_NORMAL;
    if (room->inn_arrival) {
        /* The exterior is black for 24 field updates after the room changes.
         * Its entrance then fades in for 17 updates before movement resumes. */
        ++room->inn_arrival;
        if (room->inn_arrival <= 24) {
            if (room->inn_arrival == 24)
                bfm_village_arrival_lower_register(room->village, room->run);
            ++room->update;
            return BFM_BEDROOM_ADVANCED;
        }
        if (room->inn_arrival < 42) {
            bfm_village_arrival_lower_ambient(room->village, room->run);
            return submit_village_frame(room) ? BFM_BEDROOM_ADVANCED : 0;
        }
        room->inn_arrival = 0;
    }
    if (room->village) return village_step(room, confirm_edge, reveal_edge, direction,
                                            accept_edge, field);
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
            unsigned command = 0;
            int selection = bfm_library_books_input(room->books, direction, held, accept_edge, back_edge,
                                      room->entry.state.variables[98], &command);
            queue_effect(room, BFM_ROOM_SCENE_BANK, command);
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
        if (choice) queue_effect(room, BFM_ROOM_SCENE_BANK, accept_edge ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND);
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
        queue_effect(room, BFM_ROOM_SCENE_BANK, BFM_ROOM_AWARD_SOUND);
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
    if (current_field_retry(room)) return NULL;
    if (room->village && (bfm_village_arrival_player(room->village)->events & BFM_VILLAGE_HIT))
        return room->landing_sound;
    return NULL;
}

BfmCableSound *bfm_bedroom_slider(const BfmBedroom *room, int *event)
{
    *event=room->slider_event;
    return room->slider_sound;
}

size_t bfm_bedroom_take_effects(BfmBedroom *room, const BfmBedroomSound **events)
{
    *events = room->sounds;
    if (room->sounds_taken) return 0;
    room->sounds_taken = 1;
    return room->sound_count;
}

BfmBedroomEffects *bfm_bedroom_effect(const BfmBedroom *room, unsigned *command)
{
    const BfmBedroomSound *last = room->sound_count ? &room->sounds[room->sound_count-1] : NULL;
    *command = last ? last->command : 0;
    return last ? last->effects : room->effects;
}

int bfm_bedroom_take_effect_release(BfmBedroom *room)
{
    int release = room->release_room_effects;
    room->release_room_effects = 0;
    return release;
}

unsigned bfm_bedroom_take_score_command(BfmBedroom *room)
{
    return room ? bfm_village_arrival_take_score_command(room->village) : 0;
}

BfmForestScore *bfm_bedroom_score(const BfmBedroom *room)
{
    return room ? room->current_score : NULL;
}

BfmRetryAudioEvent bfm_bedroom_retry_audio(BfmBedroom *room)
{
    if (!room) return BFM_RETRY_AUDIO_NONE;
    BfmRetryAudioEvent event = room->retry_audio;
    room->retry_audio = BFM_RETRY_AUDIO_NONE;
    return event;
}

int bfm_bedroom_retry_score_result(BfmBedroom *room, BfmRetryAudioEvent event, int accepted)
{
    return bfm_retry_screen_woods_score_result(current_field_retry(room), event, accepted);
}

BfmWoodsRetryPage bfm_bedroom_retry_destination(const BfmBedroom *room)
{
    return bfm_retry_screen_woods_destination(current_field_retry(room));
}

int bfm_bedroom_prepare_woods_recovery(BfmBedroom *room, BfmBedroomEntry *out)
{
    return room && bfm_village_arrival_prepare_recovery(room->village, room->run, out);
}

int bfm_bedroom_step_axes(BfmBedroom *room, int confirm_edge, int reveal_edge,
                          int direction, int held, int accept_edge, int back_edge,
                          int x, int y, int talk_edge)
{
    return bfm_bedroom_step_field(room, confirm_edge, reveal_edge, direction, held, accept_edge,
                                  back_edge, (BfmFieldInput){.x=x, .y=y, .square_pressed=talk_edge});
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

int bfm_bedroom_draw_counter(BfmBedroom *room, int dialogue, uint32_t pixels[240][320])
{
    if (room->bakery) {
        if (!bfm_bakery_field_draw(room->bakery, pixels)) return 0;
        memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
        bfm_bakery_field_draw_hud(room->bakery, room->hud, &room->status, pixels);
        bfm_bakery_field_shade(room->bakery, pixels);
        return 1;
    }
    if (!room->inn || !bfm_inn_field_draw(room->inn, dialogue, pixels)) return 0;
    if (dialogue) {
        int services = bfm_inn_field_service(room->inn);
        if (services) bfm_castle_hud_command(room->hud);
        else bfm_castle_hud_command_closed(room->hud);
        memcpy(room->status.record, room->entry.state.progress, sizeof room->status.record);
        bfm_forest_hud_draw(room->hud, &room->status, pixels);
        if (services) bfm_inn_hud_draw_fatigue(room->hud, &room->status, pixels);
    }
    bfm_inn_field_shade(room->inn, pixels);
    return 1;
}

uint16_t bfm_bedroom_display_room(const BfmBedroom *room)
{
    if (room->castle_tick >= 30) return (uint16_t)half(room->castle_door);
    if (room->audience) return 0x3003;
    if (room->nightmare) return 0x3012;
    if (room->bakery) return 0x2015;
    if (room->inn) return 0x2017;
    if (room->village) return bfm_village_arrival_room(room->village);
    if (room->cable) return 0x3004;
    if (room->command_room) return 0x3000;
    if (room->library) return 0x3002;
    return room->field && half(room->entry.state.destination) == 0x3001 ? 0x3001 : 0;
}

int bfm_bedroom_draw(BfmBedroom *room, uint32_t pixels[240][320])
{
    if (room->castle_tick) {
        memcpy(pixels, room->frames[room->presented], sizeof room->frames[0]);
        unsigned tick = room->castle_tick;
        BfmForestGatewayFrame fade = {.view = BFM_GATEWAY_HELD_FOREST,
                                      .shade = tick > 41 ? (tick-41)/2 : 0};
        bfm_forest_gateway_transition_shade(&fade, pixels);
        return 1;
    }
    if (room->audience) {
        if (!bfm_castle_audience_draw(room->audience, room->hud, &room->status, pixels)) return 0;
        BfmForestGatewayFrame fade = {.shade = room->castle_reveal * 2};
        bfm_forest_gateway_transition_shade(&fade, pixels);
        return 1;
    }
    if (room->shop_door) {
        unsigned tick = room->shop_entry_tick;
        int bakery = half(room->shop_door) == 0x2015;
        unsigned clear = bakery ? BAKERY_CLEAR : INN_CLEAR;
        unsigned walk = bakery ? BAKERY_WALK : INN_WALK;
        unsigned full_light = bakery ? BAKERY_FULL_LIGHT : INN_FULL_LIGHT;
        BfmForestGatewayFrame fade = {0};
        if (tick < clear) {
            memcpy(pixels, room->frames[room->presented], sizeof room->frames[0]);
            if (tick > SHOP_FADE_OUT) {
                fade.shade = (tick - SHOP_FADE_OUT) * 2;
                if (fade.shade > 30) fade.shade = 30;
            }
        } else if (tick < walk) {
            fade.view = BFM_GATEWAY_BLACK;
            fade.shade = 31;
        } else {
            if (!bfm_bedroom_draw_counter(room, 1, pixels)) return 0;
            fade.shade = (full_light - tick) * 2 - 1;
        }
        bfm_forest_gateway_transition_shade(&fade, pixels);
        return 1;
    }
    if (room->inn || room->bakery) return bfm_bedroom_draw_counter(room, 1, pixels);
    memcpy(pixels, room->frames[room->presented], sizeof room->frames[0]);
    if (room->village && bfm_village_arrival_draw_woods_crossing(room->village, pixels)) return 1;
    if (room->nightmare || room->recovered_exterior) {
        bfm_nightmare_transition_draw(&room->recovery_transition, pixels);
        return 1;
    }
    unsigned crossing = room->village ? bfm_village_arrival_transfer_tick(room->village) : 0;
    if (bfm_village_arrival_maze_crossing(room->village)) {
        bfm_forest_maze_wipe_shade(crossing,bfm_village_arrival_reveal_tick(room->village),pixels);
        return 1;
    }
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
            unsigned reveal = crossing ? bfm_village_arrival_reveal_tick(room->village) : 0;
            for (unsigned y=0;y<240;++y) {
                unsigned band=(y<120 ? y : 239-y)/8;
                int loss=entering ? entry_loss : (int)((tick-wipe)/2)-(int)band;
                if (crossing) {
                    loss = crossing >= reveal ? 35 - 2*(int)(crossing - reveal) :
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
    if (room->audience && bfm_castle_audience_guest_pending(room->audience))
        return "This castle guest visit is unfinished. Start to return.";
    if (unmounted_box_endpoint(room)) return "This Memory Box destination is unfinished. Start to return.";
    if (room->inn) return "Inn preview complete. Start to return.";
    return room->village_trip ? "Cable ride complete. Start to return." : "Library preview complete. Start to return.";
}

int bfm_bedroom_endpoint(const BfmBedroom *room)
{
    if (room->castle_tick || room->nightmare) return 0;
    if (room->audience) return bfm_castle_audience_guest_pending(room->audience);
    return unmounted_box_endpoint(room) || bfm_inn_field_finished(room->inn) ||
           (!room->village_trip && room->message_closed && room->drain==2);
}

struct ForestVoice *bfm_bedroom_voice(BfmBedroom *room, unsigned *generation)
{
    if (room->audience) return bfm_castle_audience_voice(room->audience, generation);
    if (room->nightmare) {
        *generation = bfm_nightmare_room_generation(room->nightmare);
        return bfm_nightmare_room_voice(room->nightmare);
    }
    BfmRetryScreen *retry = current_field_retry(room);
    if (retry) {
        *generation = bfm_retry_screen_generation(retry);
        return bfm_retry_screen_voice(retry);
    }
    *generation = room->generation;
    return current_voice(room);
}

unsigned bfm_bedroom_voice_ready_field(const BfmBedroom *room)
{
    return room->voice_ready_field;
}

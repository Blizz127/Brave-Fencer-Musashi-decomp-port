/* SPDX-License-Identifier: MIT */
#include "village_illness_lower.h"
#include "bedroom_award.h"
#include "bedroom_effects.h"
#include "field_forced_bag.h"
#include "forest_message_art.h"
#include "forest_voice.h"
#include "village_illness_grocery.h"
#include "village_illness_grocery_cast.h"
#include "village_illness_wid.h"
#include "village_illness_mine_gate.h"
#include "village_illness_mine_hero.h"
#include "village_portrait.h"
#include "village_talk_camera.h"
#include "scene_tiles.h"
#include "village_camera.h"
#include "village_clock.h"
#include "village_field.h"
#include "village_lighting.h"
#include "village_mayor_door.h"
#include "village_sky.h"
#include "village_street_scenery.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t street_room = UINT32_C(0x80190780);
enum { STREET_MEMBER = 124, INN_ENTRANCE = 10, MINE_ENTRANCE = 15, REVEAL_FIELDS = 1 + 16 };

struct BfmVillageIllnessLower {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    BfmSaveRecord *save;
    uint16_t (*tiles)[1024];
    BfmVillageField field;
    BfmVillageMotion *motion;
    BfmVillagePlayer player;
    BfmVillageCamera camera, presented;
    BfmVillageTalkCamera talk_camera;
    BfmVillageGrocery *grocery;
    BfmVillageIllnessGroceryCast *cast;
    BfmVillageWid *wid;
    BfmVillageMineGate *mine_gate;
    BfmVillageMineHero *mine_hero;
    ForestVoice *mine_voice;
    unsigned generation, stopped_generation, closed_answer;
    int voice_stopped, camera_restored;
    BfmVillageIllnessWindmill *windmill;
    BfmFieldForcedBag bag;
    BfmFieldItemMenuArt *bag_art;
    BfmForestMessage *window;
    BfmForestMessageArt *art;
    const MusashiDiscMedia *disc;
    const uint8_t *resident, *award_geometry;
    size_t resident_bytes;
    unsigned speaker_portrait, sounds[4], sound_count, award_age, menu_score_command;
    int bag_closed, bag_serviced, award_active, speaker_x, right, closed;
    int16_t award_xyz[3];
    BfmVillageCameraTurn turn;
    BfmVillageStreetScenery *scenery;
    BfmVillageShopDoors *doors;
    BfmVillageDaylight daylight;
    BfmFieldRest rest;
    unsigned reveal_fields;
};

static const uint8_t *mounted(const BfmVillageIllnessLower *street, uint32_t address, size_t count)
{
    if (address < scene_base || address - scene_base > street->bytes ||
        count > street->bytes - (address - scene_base)) return NULL;
    return street->scene + (address - scene_base);
}

static int mount_camera(BfmVillageIllnessLower *street, unsigned entrance)
{
    const uint8_t *room = mounted(street, street_room, 0xcc);
    const uint8_t *entries = room ? mounted(street, u32le(room + 0xc8), 4 * (entrance + 1)) : NULL;
    const uint8_t *door = entries ? mounted(street, u32le(entries + 4 * entrance), 36) : NULL;
    const uint8_t *angles = mounted(street, UINT32_C(0x8018aecc), 8 * 2);
    uint32_t reverse = entrance == MINE_ENTRANCE ? UINT32_C(0x02003043) : UINT32_C(0x06002058);
    if (!door || u32le(door) != reverse || door[3] >= 8 || !angles) return 0;
    unsigned yaw = u16le(angles + 2 * door[3]);
    if (yaw >= 4096) return 0;
    bfm_village_camera_lower(&street->camera, street->player.root.xyz, 1);
    street->turn.goal = street->camera.yaw = (int)yaw;
    bfm_village_camera_lower(&street->camera, street->player.root.xyz, 0);
    return 1;
}

void bfm_village_illness_lower_free(BfmVillageIllnessLower *street)
{
    if (!street) return;
    bfm_village_mine_hero_free(street->mine_hero);
    bfm_village_mine_gate_free(street->mine_gate);
    bfm_forest_voice_free(street->mine_voice);
    bfm_village_wid_free(street->wid);
    bfm_village_illness_windmill_free(street->windmill);
    bfm_field_item_menu_art_free(street->bag_art);
    bfm_forest_message_free(street->window);
    bfm_forest_message_art_free(street->art);
    bfm_village_grocery_free(street->grocery);
    bfm_village_illness_grocery_cast_free(street->cast);
    bfm_village_shop_doors_free(street->doors);
    bfm_village_street_scenery_free(street->scenery);
    bfm_village_field_clear(&street->field);
    bfm_village_motion_free(street->motion);
    free(street);
}

BfmVillageIllnessLower *bfm_village_illness_lower_open(const MusashiDiscMedia *disc,
    BfmBedroomEntry *entry, const uint8_t *main, size_t bytes, uint16_t tiles[512][1024],
    BfmForestHud **hud)
{
    if (!disc || !entry || !main || !tiles || entry->scene_member != STREET_MEMBER) return NULL;
    uint32_t request = u32le(entry->state.destination);
    if (request != UINT32_C(0x000a1052) && request != UINT32_C(0x000f1052)) return NULL;
    unsigned entrance = entry->state.destination[2];
    BfmVillageIllnessLower *street = calloc(1, sizeof *street);
    if (!street) return NULL;
    street->tiles = tiles;
    street->save = &entry->state;
    street->disc = disc;
    street->resident = main;
    street->resident_bytes = bytes;
    street->bytes = bfm_read_archive_field_scene(disc, "SC03.CD", STREET_MEMBER, street->scene);
    street->motion = entrance == INN_ENTRANCE ?
        bfm_village_motion_resume(street->scene, street->bytes, street_room, main, bytes) :
        bfm_village_motion_return_to_street(street->scene, street->bytes, street_room, main, bytes, entrance);
    if (!street->motion) goto fail;
    street->player = *bfm_village_motion_player(street->motion);
    street->player.controlled = 0;
    street->reveal_fields = REVEAL_FIELDS;
    if (!mount_camera(street, entrance) ||
        !bfm_village_field_load_room(&street->field, disc, street->scene, street->bytes, street_room,
                                     UINT32_C(0x8018c9dc))) goto fail;
    street->scenery = bfm_village_street_scenery_load(street->scene, street->bytes, street_room);
    street->doors = bfm_village_shop_doors_load(street->scene, street->bytes, street_room);
    street->mine_gate = bfm_village_mine_gate_open(street->scene, street->bytes, street->save);
    if (!street->mine_gate) goto fail;
    street->grocery = bfm_village_grocery_open(street->scene, street->bytes);
    street->cast = bfm_village_illness_grocery_cast_open(street->scene, street->bytes, street->save);
    street->wid = bfm_village_wid_open(street->scene, street->bytes);
    street->windmill = bfm_village_illness_windmill_open(street->scene, street->bytes, street->save);
    street->award_geometry = mounted(street, UINT32_C(0x8018ddf8), 32);
    if (!street->grocery || !street->cast || !street->wid || !street->windmill ||
        !street->award_geometry || !street->scenery || !street->doors ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", STREET_MEMBER, tiles) ||
        !bfm_village_illness_light_begin(&street->daylight, street->scene, street->bytes,
                                        street->save->progress[0x37], tiles) ||
        !bfm_village_motion_lower_orb(street->motion, street->save->progress[0x48], tiles))
        goto fail;
    street->art = bfm_castle_message_art_load(disc, street->scene, street->bytes, tiles, 0x63128);
    if (!street->art) goto fail;
    street->presented = street->camera;
    bfm_bedroom_field_viewpoint(street->field.field, street->presented.eye, street->presented.target, 1, 1);
    if (hud) {
        BfmForestHud *mounted_hud = bfm_village_illness_hud_load(disc, street->scene, street->bytes, tiles);
        if (!mounted_hud) goto fail;
        *hud = mounted_hud;
    }
    bfm_village_street_scenery_step(street->scenery, 0, u32le(street->save->progress + 8));
    return street;
fail:
    bfm_village_illness_lower_free(street);
    return NULL;
}

const BfmVillagePlayer *bfm_village_illness_lower_player(const BfmVillageIllnessLower *street)
{
    return street ? &street->player : NULL;
}

const uint8_t *bfm_village_illness_lower_door(const BfmVillageIllnessLower *street)
{
    return street ? bfm_village_motion_transfer(street->motion) : NULL;
}

int bfm_village_illness_lower_rest_hud(const BfmVillageIllnessLower *street, BfmForestHud *hud)
{
    if (!street || !hud) return 0;
    unsigned equipment = street->save->progress[0x48], palette;
    if (bfm_village_hud_step_blade(hud, (int)equipment, &palette) &&
        !bfm_village_motion_lower_orb(street->motion, palette, street->tiles)) return 0;
    bfm_village_hud_rest(hud, (equipment & 0x80) != 0, street->save->progress[0x47],
                         bfm_field_rest_sleeping(&street->rest));
    return 1;
}

const BfmForestMessageFrame *bfm_village_illness_lower_page(const BfmVillageIllnessLower *street)
{
    return street && street->window ? bfm_forest_message_frame(street->window) : NULL;
}

static int resident_portrait(BfmVillageIllnessLower *street, unsigned face)
{
    if (!face || street->speaker_portrait == face) return 1;
    switch (face) {
    case 18: bfm_village_hotelo_portrait_load(street->tiles); break;
    case 21: bfm_village_mayor_portrait_load(street->tiles); break;
    case 23: bfm_village_wid_portrait_load(street->tiles); break;
    default: return 0;
    }
    bfm_field_message_art_refresh_speaker(street->art, street->tiles);
    street->speaker_portrait = face;
    return 1;
}

int bfm_village_illness_lower_bag_active(const BfmVillageIllnessLower *street)
{
    return street && street->bag.phase != BFM_BAG_IDLE;
}

int bfm_village_illness_lower_menu_serviced(const BfmVillageIllnessLower *street)
{
    return street && street->bag_serviced;
}

unsigned bfm_village_illness_lower_take_score_command(BfmVillageIllnessLower *street)
{
    if (!street) return 0;
    unsigned command = street->menu_score_command;
    street->menu_score_command = 0;
    return command;
}

unsigned bfm_village_illness_lower_take_sounds(BfmVillageIllnessLower *street, unsigned sounds[2])
{
    if (!street) return 0;
    unsigned count = street->sound_count > 2 ? 2 : street->sound_count;
    memcpy(sounds, street->sounds, count * sizeof *sounds);
    street->sound_count -= count;
    memmove(street->sounds, street->sounds + count, street->sound_count * sizeof *sounds);
    return count;
}

ForestVoice *bfm_village_illness_lower_voice(const BfmVillageIllnessLower *street, unsigned *generation)
{
    if (!street) return NULL;
    if (generation) *generation = street->generation;
    return street->mine_voice;
}

int bfm_village_illness_lower_take_voice_stop(BfmVillageIllnessLower *street, unsigned *generation)
{
    if (!street || !generation || !street->voice_stopped) return 0;
    *generation = street->stopped_generation;
    street->voice_stopped = 0;
    return 1;
}

static int service_mine_gate(BfmVillageIllnessLower *street, BfmFieldInput input, int available,
                             BfmGameplayRun *run)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(street->motion);
    if (street->mine_hero && !bfm_village_mine_hero_step(street->mine_hero, &street->presented)) return 0;
    BfmMineGateFrame gate;
    BfmMineGateResult result = bfm_village_mine_gate_step(street->mine_gate, player->root.xyz,
        available && bfm_village_motion_can_examine(street->motion), input.square_pressed,
        street->closed, street->closed_answer, bfm_village_mine_hero_ready(street->mine_hero),
        street->camera_restored, street->save, run, &gate);
    if (result == BFM_MINE_GATE_ERROR) return 0;
    if (result == BFM_MINE_GATE_FREE) return 1;
    if (!street->mine_hero) {
        if (!bfm_village_motion_examine_tick(street->motion, run)) return 0;
        street->mine_hero = bfm_village_mine_hero_open(street->scene, street->bytes, street->motion);
        if (!street->mine_voice) street->mine_voice = bfm_mine_gate_voice_load(street->disc);
        if (!street->mine_hero || !street->mine_voice) return 0;
    }
    if (!bfm_village_mine_hero_orders(street->mine_hero, &gate, &street->presented)) return 0;
    if (gate.camera_frame) {
        street->camera_restored = 0;
        if (!bfm_village_talk_camera_frame(&street->talk_camera, gate.camera_station, gate.camera_frame,
                                           &street->presented)) return 0;
    }
    if (gate.release_camera) bfm_village_talk_camera_release(&street->talk_camera);
    if (gate.opened) {
        street->window = gate.opened;
        street->speaker_x = 0;
        street->right = 0;
    }
    if (gate.sound) street->sounds[street->sound_count++] = gate.sound;
    if (result == BFM_MINE_GATE_RELEASED) {
        if (!bfm_village_mine_hero_release(street->mine_hero)) return 0;
        bfm_village_mine_hero_free(street->mine_hero);
        street->mine_hero = NULL;
    }
    return 1;
}

static int service_residents(BfmVillageIllnessLower *street, BfmFieldInput input,
                              BfmGameplayRun *run, unsigned tick)
{
    BfmVillageGroceryFrame grocery = {0};
    BfmVillageWidFrame wid = {0};
    const BfmBedroomView *view = bfm_bedroom_field_view(street->field.field);
    BfmVillageGroceryResult result = BFM_GROCERY_FREE;
    int mine_busy = bfm_village_mine_gate_active(street->mine_gate);
    bfm_village_motion_mine_gate(street->motion, street->mine_gate, tick);
    if (mine_busy) result = BFM_GROCERY_LOCKED;
    else if (!bfm_village_grocery_active(street->grocery))
        result = bfm_village_wid_step(street->wid, street->windmill, street->motion, view,
            input.square_pressed, street->closed, street->bag_closed, street->save, run, &wid);
    street->bag_closed = 0;
    if (result == BFM_GROCERY_FREE) {
        unsigned story = u32le(street->save->progress + 8);
        int ready = bfm_village_illness_grocery_cast_ready(street->cast, story);
        result = bfm_village_grocery_step(street->grocery, street->motion, view,
            input.square_pressed, ready, street->closed, street->save, run, &grocery);
    }
    if (result == BFM_GROCERY_ERROR) return 0;
    if (grocery.opened || wid.opened) {
        street->window = wid.opened ? wid.opened : grocery.opened;
        street->speaker_x = wid.opened ? wid.speaker_x : grocery.speaker_x;
        street->right = wid.opened ? wid.portrait_on_right : grocery.portrait_on_right;
    }
    if (!resident_portrait(street, wid.resident_portrait ? wid.resident_portrait : grocery.resident_portrait)) return 0;
    if (grocery.camera_frame && !bfm_village_talk_camera_frame(&street->talk_camera,
        grocery.camera_station, grocery.camera_frame, &street->presented)) return 0;
    if (wid.start_camera && !bfm_village_talk_camera_world(&street->talk_camera,
        wid.camera_eye, wid.camera_target, &street->presented, &street->presented)) return 0;
    if (grocery.release_camera || wid.release_camera) bfm_village_talk_camera_release(&street->talk_camera);
    /* The physical Square scan samples the current foot before ordinary
     * steering. Other residents have already had their priority here. */
    if (!service_mine_gate(street, input, result == BFM_GROCERY_FREE, run)) return 0;
    if (result == BFM_GROCERY_FREE && !street->mine_hero &&
        !bfm_village_motion_step_field(street->motion, input.x, input.y,
                                      (street->camera.yaw + 2048) & 4095, run)) return 0;
    if (!bfm_village_illness_grocery_cast_step(street->cast, street->save, tick,
        bfm_village_grocery_active(street->grocery), grocery.mayor_program, grocery.mayor_turn,
        grocery.mayor_yaw, grocery.hotelo_command) ||
        !bfm_village_illness_windmill_step(street->windmill, street->save,
            bfm_village_wid_windmill_active(street->wid), &wid.cast)) return 0;
    street->closed = 0;
    unsigned hinge = bfm_village_illness_windmill_door_sound(street->windmill);
    /* The ordinary porch body sends its own knock on the hand-contact field. */
    if (wid.sound && wid.sound != BFM_VILLAGE_KNOCK_SOUND) street->sounds[street->sound_count++] = wid.sound;
    if (hinge) street->sounds[street->sound_count++] = hinge;
    if (wid.pickup_effect) {
        street->award_active = 1;
        street->award_age = 0;
        memcpy(street->award_xyz, bfm_village_motion_player(street->motion)->root.xyz, sizeof street->award_xyz);
    } else if (street->award_active && ++street->award_age > 40) street->award_active = 0;
    if (wid.request_inventory) {
        if (!street->bag_art) street->bag_art = bfm_field_item_menu_art_open(street->disc,
            street->scene, street->bytes, 0x63128, street->resident, street->resident_bytes, street->tiles);
        if (!street->bag_art) return 0;
        bfm_field_forced_bag_begin(&street->bag);
    }
    return 1;
}

int bfm_village_illness_lower_step(BfmVillageIllnessLower *street, BfmGameplayRun *run,
                                  BfmFieldInput input, unsigned scene_tick)
{
    if (!street) return 0;
    street->sound_count = 0;
    street->bag_serviced = bfm_village_illness_lower_bag_active(street);
    if (street->bag_serviced) {
        if (street->bag.phase == BFM_BAG_OPEN && !street->bag.fields)
            street->menu_score_command = BFM_ROOM_MENU_DUCK;
        street->bag_closed = bfm_field_forced_bag_step(&street->bag, input, street->resident,
            street->resident_bytes, street->save, run, street->sounds);
        if (street->bag_closed) street->menu_score_command = BFM_ROOM_MENU_RESTORE;
        street->sound_count = !!street->sounds[0] + !!street->sounds[1];
        return 1;
    }
    if (street->mine_voice) bfm_forest_voice_step(street->mine_voice);
    BfmForestMessage *prior = street->window;
    int occupied = bfm_village_grocery_active(street->grocery) || bfm_village_wid_active(street->wid) ||
                   bfm_village_mine_gate_active(street->mine_gate);
    int shot = street->talk_camera.phase != 0;
    if (shot) {
        street->presented = street->camera;
        bfm_village_talk_camera_step(&street->talk_camera, &street->presented);
        bfm_bedroom_field_viewpoint(street->field.field, street->presented.eye, street->presented.target, 1, 1);
        if (!street->talk_camera.phase) {
            bfm_village_grocery_camera_restored(street->grocery);
            bfm_village_wid_camera_restored(street->wid);
            street->camera_restored = 1;
        }
    }
    int entering = street->reveal_fields != 0;
    int departing = bfm_village_illness_lower_door(street) != NULL;
    int asleep = 0;
    if (!departing) {
        /* The receiving operation services the ordinary standing actor with
         * a neutral pad through its final reveal field. */
        if (entering) {
            input = (BfmFieldInput){0};
            --street->reveal_fields;
        } else if (!occupied) {
            bfm_field_rest_input(&street->rest, street->save, input);
        }
        bfm_village_motion_woods_condition(street->motion, bfm_field_tired_stance(street->save->progress));
        asleep = bfm_field_rest_sleeping(&street->rest);
        if (street->rest.pose == BFM_FIELD_REST_FREE) {
            if (!service_residents(street, input, run, scene_tick)) return 0;
        } else {
            BfmVillageWindmillCue idle_cue = {0};
            if (!bfm_village_illness_grocery_cast_step(street->cast, street->save, scene_tick, 0, 0, 0, 0, 0) ||
                !bfm_village_illness_windmill_step(street->windmill, street->save, 0, &idle_cue)) return 0;
            int pose = bfm_village_motion_field_rest(street->motion, street->rest.pose, street->rest.ticks);
            if (!pose) return 0;
            if (pose == 2 && street->rest.pose == BFM_FIELD_REST_WAKE) street->save->progress[0x47] = 0;
            bfm_field_rest_advance(&street->rest, pose == 2);
        }
        street->player = *bfm_village_motion_player(street->motion);
        departing = bfm_village_illness_lower_door(street) != NULL;
        street->player.controlled = !entering && !departing && street->player.controlled;
        if (!bfm_village_motion_face(street->motion, street->tiles)) return 0;
        /* The bell and vane share the ordinary actor pass. The outgoing
         * operation holds both children while the street camera still runs. */
        bfm_village_street_scenery_step(street->scenery, scene_tick, u32le(street->save->progress + 8));
        bfm_village_street_vane_step(street->scenery, run);
    }

    int in_scene = bfm_village_grocery_active(street->grocery) || bfm_village_wid_active(street->wid) ||
                   bfm_village_mine_gate_active(street->mine_gate);
    int turn = !occupied && !in_scene && street->player.controlled && street->rest.pose == BFM_FIELD_REST_FREE ?
               input.camera_turn : 0;
    bfm_village_camera_turn(&street->camera, &street->turn, turn);
    bfm_village_camera_lower(&street->camera, street->player.root.xyz, 0);
    if (!shot) street->presented = street->camera;
    bfm_bedroom_field_viewpoint(street->field.field, street->presented.eye, street->presented.target, 1, 1);
    bfm_village_daylight_step(&street->daylight, street->save->progress[0x37], street->tiles);
    if (!entering && !departing && !occupied) {
        bfm_field_rest_stamina(street->save, asleep);
        bfm_village_clock_step(street->save, asleep);
    }
    if (prior && street->window == prior) {
        int confirm = input.cross_pressed || input.triangle_pressed || input.circle_pressed;
        int reveal = input.circle_pressed;
        int direction = (input.pressed_directions & 0x10) ? -1 : !!(input.pressed_directions & 0x40);
        if (bfm_field_message_choose(prior, direction, input.circle_pressed || input.triangle_pressed,
                                     input.cross_pressed))
            street->sounds[street->sound_count++] = input.cross_pressed ? BFM_ROOM_ACCEPT_SOUND : BFM_ROOM_MOVE_SOUND;
        unsigned previous = street->generation;
        const BfmForestMessageFrame *page = street->mine_voice ?
            bfm_forest_voice_page(street->mine_voice, prior, &street->generation, confirm, reveal) :
            bfm_field_message_tick(prior, 0, confirm, reveal);
        if (!page || (!street->mine_voice && (page->events & BFM_FOREST_VOICE_REQUEST))) return 0;
        if (page->events & BFM_FOREST_VOICE_STOP) {
            street->voice_stopped = 1;
            street->stopped_generation = previous;
        }
        if (page->events & BFM_FOREST_MESSAGE_CLOSED) {
            street->closed_answer = page->choices ? page->answer : 0;
            bfm_forest_message_free(prior);
            street->window = NULL;
            street->closed = 1;
        }
    }
    return 1;
}

static void draw_to(void *context, int depth)
{
    BfmVillageIllnessLower *street = context;
    for (;;) {
        int next = bfm_village_shop_doors_next_depth(street->doors);
        int player = bfm_bedroom_figure_next_depth(street->field.hero);
        int scenery = bfm_village_street_scenery_next_depth(street->scenery);
        if (player > next) next = player;
        if (scenery > next) next = scenery;
        if (next == INT_MIN || next < depth) break;
        bfm_village_shop_doors_draw_to(street->doors, next);
        bfm_bedroom_figure_draw_to(street->field.hero, next);
        bfm_village_street_scenery_draw_to(street->scenery, next);
    }
    if (depth == INT_MIN) bfm_bedroom_figure_draw_to(street->field.hero, depth);
}

int bfm_village_illness_lower_draw(BfmVillageIllnessLower *street, unsigned scene_tick,
    const uint8_t flags[64], unsigned draw_page, uint32_t pixels[240][320])
{
    if (!street || !flags || !pixels ||
        !bfm_lower_street_sky_draw_gain(street->daylight.sky, pixels) ||
        !bfm_bedroom_figure_lights(street->field.hero, street->daylight.lamps) ||
        !bfm_village_field_prepare(&street->field, &street->presented, &street->player.root,
                                   &street->player.pose, flags, scene_tick, street->tiles, &pixels[0][0]))
        return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(street->field.field);
    bfm_village_street_scenery_lights(street->scenery, street->daylight.lamps);
    bfm_village_shop_doors_set_windmill(street->doors,
        bfm_village_illness_windmill_hinged(street->windmill), bfm_village_illness_windmill_door_angle(street->windmill));
    bfm_village_shop_doors_prepare(street->doors, view, street->tiles, &pixels[0][0], street->save);
    if (!bfm_village_street_scenery_prepare(street->scenery, view, street->tiles, &pixels[0][0], draw_page) ||
        !bfm_village_street_vane_add(street->scenery, street->field.hero, view) ||
        !bfm_village_mine_gate_draw(street->mine_gate, street->field.hero, view) ||
        !bfm_village_illness_grocery_cast_add(street->cast, street->field.hero, view) ||
        !bfm_village_illness_windmill_add(street->windmill, street->field.hero, view)) return 0;
    bfm_bedroom_field_draw(street->field.field, street->tiles, &pixels[0][0], draw_to, street);
    if (street->award_active)
        bfm_bedroom_award_draw_slots(street->award_geometry, view, street->award_xyz, street->award_age, 15, pixels);
    return 1;
}

int bfm_village_illness_lower_draw_window(const BfmVillageIllnessLower *street, uint32_t pixels[240][320])
{
    if (bfm_village_illness_lower_bag_active(street))
        return bfm_field_forced_bag_draw(&street->bag, street->bag_art, street->save, street->tiles, pixels);
    const BfmForestMessageFrame *page = bfm_village_illness_lower_page(street);
    return !page || bfm_forest_message_art_draw_placed(street->art, page, (int16_t)street->speaker_x,
                                                       street->right, pixels);
}

/* SPDX-License-Identifier: MIT */
#include "bakery_field.h"
#include "bakery_room_art.h"
#include "bakery_service.h"
#include "forest_gateway_transition.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { MEMBER_OFFSET = 0x801ef468u - 0x80128158u, COUNTER_WALK = 0xbb8,
       JAM_STATION = 0xb84, EXIT_POSITION = 0xc, EXIT_FADE = 21, EXIT_SELECT = 35 };

struct BfmBakeryField {
    BfmBakeryRoomArt *room;
    BfmForestMessageArt *portraits;
    BfmBakeryMenuArt *labels;
    BfmSaveArt *font;
    BfmBedroomTravel *travel;
    BfmBedroomMotion *motion;
    BfmBedroomPose pose;
    BfmBakerySpeech *speech;
    BfmBakeryService service;
    BfmSaveRecord *record;
    const uint8_t *scene, *module, *main, *outside;
    size_t module_bytes, main_bytes;
    uint16_t (*tiles)[1024];
    unsigned age, exit_age, sound;
    uint16_t *service_clock, drawn_clock;
    int speech_updated;
};

void bfm_bakery_field_free(BfmBakeryField *shop)
{
    if (!shop) return;
    bfm_bakery_speech_free(shop->speech);
    bfm_bakery_menu_art_free(shop->labels);
    free(shop->font);
    bfm_forest_message_art_free(shop->portraits);
    bfm_bedroom_motion_free(shop->motion);
    bfm_bedroom_travel_free(shop->travel);
    bfm_bakery_room_art_free(shop->room);
    free(shop);
}

BfmBakeryField *bfm_bakery_field_open(const MusashiDiscMedia *disc, const uint8_t *door,
                                     BfmSaveRecord *record, const uint8_t *main, size_t main_bytes,
                                     uint16_t tiles[512][1024], uint16_t *service_clock)
{
    if (!disc || !door || u16le(door) != 0x2015 || door[2] || !record || !main || !tiles || !service_clock)
        return NULL;
    BfmBakeryField *shop = calloc(1, sizeof *shop);
    if (!shop) return NULL;
    shop->room = bfm_bakery_room_art_open(disc, tiles);
    if (!shop->room) goto fail;
    size_t bytes;
    uint32_t address;
    shop->scene = bfm_bakery_room_art_scene(shop->room, &bytes, &address);
    if (!shop->scene || bytes < MEMBER_OFFSET + COUNTER_WALK + 4) goto fail;
    shop->module = shop->scene + MEMBER_OFFSET;
    shop->module_bytes = bytes - MEMBER_OFFSET;
    shop->outside = shop->module + 0xb2c;
    const uint8_t *walk = shop->module + COUNTER_WALK;
    if (u32le(shop->outside) != 0x00041010 || u32le(walk - 4) != 3) goto fail;
    shop->travel = bfm_bedroom_travel_open(shop->scene, bytes, address, 0);
    shop->motion = bfm_bedroom_motion_open(shop->scene, bytes, main, main_bytes, address);
    shop->portraits = bfm_castle_message_art_load(disc, shop->scene, bytes, tiles, 0x65a84);
    shop->labels = bfm_bakery_menu_art_load(disc, shop->scene, MEMBER_OFFSET);
    shop->font = bfm_save_art_load(disc);
    if (!shop->travel || !shop->motion || !shop->portraits || !shop->labels || !shop->font ||
        !bfm_grillin_blade_palette(shop->motion, record->progress[0x48], tiles) ||
        !bfm_bedroom_motion_step(shop->motion, BFM_BEDROOM_REST, &shop->pose) ||
        !bfm_bedroom_travel_walk(shop->travel, s16le(walk), s16le(walk + 2), 0)) goto fail;
    shop->record = record;
    shop->main = main;
    shop->main_bytes = main_bytes;
    shop->tiles = tiles;
    shop->service_clock = service_clock;
    return shop;
fail:
    bfm_bakery_field_free(shop);
    return NULL;
}

static int step_counter_walk(BfmBakeryField *shop, BfmBedroomAction *action)
{
    if (!bfm_bedroom_travel_step(shop->travel, bfm_bakery_counter_camera.eye,
                                 bfm_bakery_counter_camera.target)) return 0;
    switch (bfm_bedroom_travel_root(shop->travel)->locomotion_edge) {
    case BFM_TRAVEL_WALK: *action = BFM_BEDROOM_WALK; break;
    case BFM_TRAVEL_FALL: *action = BFM_BEDROOM_FALL; break;
    case BFM_TRAVEL_LAND: *action = BFM_BEDROOM_LAND; break;
    case BFM_TRAVEL_STAND:
        *action = BFM_BEDROOM_REST;
        if (!shop->exit_age) {
            const uint8_t *jam = shop->module + JAM_STATION;
            bfm_bedroom_travel_face_point(shop->travel, s16le(jam), s16le(jam + 4));
        }
        break;
    default: break;
    }
    return 1;
}

int bfm_bakery_field_step(BfmBakeryField *shop, uint16_t pressed, uint16_t held_directions,
                          unsigned voice_status, BfmGameplayRun *run)
{
    if (!shop) return 0;
    shop->speech_updated = 0;
    shop->sound = 0;
    if (shop->exit_age == EXIT_SELECT) return 1;
    ++shop->age;
    if (shop->exit_age) ++shop->exit_age;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    BfmBedroomStance stance = BFM_BEDROOM_STANCE_INACTIVE;
    if (bfm_bedroom_travel_root(shop->travel)->busy) {
        if (!step_counter_walk(shop, &action)) return 0;
    } else if (!shop->speech) {
        shop->speech = bfm_bakery_speech_open(shop->module, shop->module_bytes, shop->record);
        if (!shop->speech) return 0;
        if (run) run->story_flags[270/8] |= shop->record->flags[270/8] & (1u << (270%8));
        stance = BFM_BEDROOM_STANCE_SCRIPTED;
    } else if (bfm_bakery_speech_service(shop->speech)) {
        shop->drawn_clock = (*shop->service_clock)++;
        pressed = bfm_bakery_service_pad(&shop->service, pressed, held_directions);
        BfmBakeryServiceResult result = bfm_bakery_service_step(&shop->service, pressed, shop->record, run);
        shop->sound = shop->service.sound;
        if (result == BFM_BAKERY_SERVICE_ERROR) return 0;
        if (result == BFM_BAKERY_SERVICE_QUIT) {
            if (!bfm_bakery_speech_finish_service(shop->speech)) return 0;
        } else {
            bfm_bakery_speech_service_tick(shop->speech, shop->service.caption,
                                           bfm_bakery_service_item(&shop->service, shop->record),
                                           shop->service.item, (int)(shop->service.quote_drans / 10));
        }
        return 1;
    } else if (!shop->exit_age) {
        stance = BFM_BEDROOM_STANCE_SCRIPTED;
        uint32_t previous_story = u32le(shop->record->progress + 8);
        if (!bfm_bakery_speech_step(shop->speech, pressed, voice_status)) return 0;
        if (run && previous_story != u32le(shop->record->progress + 8))
            memcpy(run->player.record + 8, shop->record->progress + 8, 4);
        shop->speech_updated = 1;
        if (bfm_bakery_speech_service(shop->speech) &&
            !bfm_bakery_service_begin(&shop->service, shop->scene, MEMBER_OFFSET, shop->main,
                                      shop->main_bytes, shop->record)) return 0;
        if (bfm_bakery_speech_leaving(shop->speech)) {
            const uint8_t *door = shop->module + EXIT_POSITION;
            if (!bfm_bedroom_travel_walk(shop->travel, s16le(door), s16le(door + 4), 0)) return 0;
            shop->exit_age = 1;
        }
    }
    return bfm_bedroom_motion_step_field(shop->motion, action, run, stance, &shop->pose) &&
           bfm_bedroom_motion_face(shop->motion, shop->tiles);
}

int bfm_bakery_field_draw(BfmBakeryField *shop, uint32_t pixels[240][320])
{
    if (!shop || !bfm_bakery_room_art_draw(shop->room, bfm_bedroom_travel_root(shop->travel),
                                           &shop->pose, shop->record->flags, shop->age, pixels)) return 0;
    if (!shop->speech) return 1;
    int service = bfm_bakery_speech_service(shop->speech);
    if (service) {
        BfmBakeryMenuView view;
        if (!bfm_bakery_service_view(&shop->service, shop->record, &view)) return 0;
        view.age = shop->drawn_clock;
        if (!bfm_bakery_menu_draw(shop->labels, shop->font, &view, pixels)) return 0;
    }
    return bfm_bakery_speech_draw(shop->speech, shop->service.caption,
                                  service ? bfm_bakery_service_item(&shop->service, shop->record) : 0,
                                  shop->service.item, (int)(shop->service.quote_drans / 10),
                                  shop->portraits, shop->scene, MEMBER_OFFSET, shop->main,
                                  shop->main_bytes, shop->tiles, pixels);
}

void bfm_bakery_field_draw_hud(const BfmBakeryField *shop, BfmForestHud *hud,
                               const BfmForestStatus *status, uint32_t pixels[240][320])
{
    int shopping = bfm_bakery_speech_service(shop->speech);
    if (shopping) bfm_castle_hud_command(hud);
    else bfm_castle_hud_command_closed(hud);
    bfm_forest_hud_draw(hud, status, pixels);
    if (shopping) bfm_inn_hud_draw_fatigue(hud, status, pixels);
}

void bfm_bakery_field_shade(const BfmBakeryField *shop, uint32_t pixels[240][320])
{
    if (shop->exit_age > EXIT_FADE) {
        BfmForestGatewayFrame fade = {.shade = 2 * (shop->exit_age - EXIT_FADE)};
        if (fade.shade > 31) fade.shade = 31;
        bfm_forest_gateway_transition_shade(&fade, pixels);
    }
}

const BfmForestMessageFrame *bfm_bakery_field_window(const BfmBakeryField *shop)
{
    return shop->speech_updated ? bfm_bakery_speech_update(shop->speech) : NULL;
}

const uint8_t *bfm_bakery_field_departure(const BfmBakeryField *shop)
{
    return shop->exit_age == EXIT_SELECT ? shop->outside : NULL;
}

unsigned bfm_bakery_field_sound(const BfmBakeryField *shop)
{
    return shop->sound;
}

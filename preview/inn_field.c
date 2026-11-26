/* SPDX-License-Identifier: MIT */
#include "inn_field.h"
#include "../pc_port/forest_player_bytes.h"
#include "inn_greeting.h"
#include "inn_keeper.h"
#include "village_field.h"
#include "forest_message_art.h"
#include "scene_tiles.h"

#include <stdlib.h>
#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
enum { KEEPER_POSITION = 0x40cc, WALK_TARGET = 0x4100, ROOM_OFFSET = 0x4218 };

struct BfmInnField {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    const uint8_t *module;
    size_t scene_size, module_size;
    uint8_t *main;
    BfmSaveRecord *record;
    BfmVillageField field;
    BfmVillageCamera camera;
    BfmBedroomTravel *travel;
    BfmBedroomMotion *motion;
    BfmBedroomPose pose;
    BfmInnKeeper *keeper;
    BfmInnGreeting *greeting;
    BfmForestMessageArt *art;
    uint16_t (*tiles)[1024];
    unsigned age, voice_status;
    int finished, updated_greeting;
};

static const uint8_t *source(const BfmInnField *inn, const uint8_t *slot, size_t count)
{
    uint32_t address = u32le(slot);
    if (address < scene_base || count > inn->scene_size ||
        address - scene_base > inn->scene_size - count) return NULL;
    return inn->scene + (address - scene_base);
}

static int mount_inn(BfmInnField *inn, const MusashiDiscMedia *disc)
{
    size_t resident = bfm_read_archive_field_scene(disc, "SC03.CD", 1, inn->scene);
    if (!resident || resident > sizeof inn->scene - 3) return 0;
    size_t offset = (resident + 3) & ~(size_t)3;
    uint8_t *module = bfm_village_event_data(disc, 76, &inn->module_size);
    if (!module) return 0;
    int fits = inn->module_size >= ROOM_OFFSET + 0xcc && inn->module_size <= sizeof inn->scene - offset;
    if (fits) {
        memcpy(inn->scene + offset, module, inn->module_size);
        inn->module = inn->scene + offset;
        inn->scene_size = offset + inn->module_size;
    }
    free(module);
    return fits;
}

void bfm_inn_field_free(BfmInnField *inn)
{
    if (!inn) return;
    bfm_inn_greeting_free(inn->greeting);
    bfm_forest_message_art_free(inn->art);
    bfm_inn_keeper_free(inn->keeper);
    bfm_bedroom_motion_free(inn->motion);
    bfm_bedroom_travel_free(inn->travel);
    bfm_village_field_clear(&inn->field);
    free(inn->main);
    free(inn);
}

BfmInnField *bfm_inn_field_open(const MusashiDiscMedia *disc, const uint8_t *transfer,
                                BfmSaveRecord *record, uint16_t tiles[512][1024])
{
    if (!disc || !transfer || !record || !tiles || u16le(transfer) != 0x2017 || transfer[2])
        return NULL;
    BfmInnField *inn = calloc(1, sizeof *inn);
    if (!inn) return NULL;
    if (!mount_inn(inn, disc)) goto fail;
    const uint8_t *room = inn->module + ROOM_OFFSET;
    uint32_t address = scene_base + (uint32_t)(room - inn->scene);
    const uint8_t *entries = source(inn, room + 0xc8, 3 * 4);
    if (!entries) goto fail;
    const uint8_t *entry = source(inn, entries, 36);
    const uint8_t *outside = source(inn, entries + 2 * 4, 36);
    if (!entry || u32le(entry) != 0x2017 || !outside || u32le(outside) != 0xa1010)
        goto fail;
    size_t main_size;
    inn->main = bfm_bedroom_player_data(disc, &main_size);
    if (!inn->main || !bfm_village_field_load_lower(&inn->field, disc, inn->scene,
                                                     inn->scene_size, address)) goto fail;
    inn->motion = bfm_bedroom_motion_open(inn->scene, inn->scene_size, inn->main, main_size, address);
    inn->travel = bfm_bedroom_travel_open(inn->scene, inn->scene_size, address, 0);
    inn->keeper = bfm_inn_keeper_load(inn->scene, inn->scene_size);
    if (!inn->motion || !inn->travel || !inn->keeper ||
        !bfm_bedroom_motion_step(inn->motion, BFM_BEDROOM_REST, &inn->pose)) goto fail;
    if (!bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", 1, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", 76, tiles) ||
        !bfm_scene_read_palettes(disc, "SC03.CD", 76, tiles)) goto fail;
    inn->art = bfm_castle_message_art_load(disc, inn->scene, inn->scene_size, tiles, 0x65a84);
    if (!inn->art) goto fail;
    const uint8_t *walk = inn->module + WALK_TARGET;
    if (u32le(walk - 4) != 3 || !bfm_bedroom_travel_walk(inn->travel, s16le(walk),
                                                        s16le(walk + 2), 0)) goto fail;
    inn->tiles = tiles;
    inn->record = record;
    inn->camera = (BfmVillageCamera){.eye = {11, -169, -561}, .target = {0, -184, -432}, .focal = 400};
    return inn;
fail:
    bfm_inn_field_free(inn);
    return NULL;
}

int bfm_inn_field_step(BfmInnField *inn, uint16_t pressed)
{
    if (!inn) return 0;
    inn->updated_greeting = 0;
    if (inn->finished) return 1;
    ++inn->age;
    const BfmBedroomRoot *player = bfm_bedroom_travel_root(inn->travel);
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    if (player->busy) {
        if (!bfm_bedroom_travel_step(inn->travel, inn->camera.eye, inn->camera.target)) return 0;
        switch (player->locomotion_edge) {
        case BFM_TRAVEL_WALK: action = BFM_BEDROOM_WALK; break;
        case BFM_TRAVEL_STAND:
            action = BFM_BEDROOM_REST;
            bfm_bedroom_travel_face_point(inn->travel, s16le(inn->module + KEEPER_POSITION),
                                          s16le(inn->module + KEEPER_POSITION + 4));
            break;
        case BFM_TRAVEL_FALL: action = BFM_BEDROOM_FALL; break;
        case BFM_TRAVEL_LAND: action = BFM_BEDROOM_LAND; break;
        default: break;
        }
    } else if (!inn->greeting) {
        inn->greeting = bfm_inn_greeting_open(inn->module, inn->module_size, inn->scene,
                                              (size_t)(inn->module - inn->scene), inn->record);
        if (!inn->greeting) return 0;
    } else {
        if (!bfm_inn_greeting_step(inn->greeting, pressed, inn->voice_status)) return 0;
        inn->updated_greeting = 1;
        /* Only accepting hospitality has a verified continuation here. */
        if (bfm_inn_greeting_leaving(inn->greeting)) inn->finished = 1;
    }
    return bfm_bedroom_motion_step(inn->motion, action, &inn->pose) &&
           bfm_bedroom_motion_face(inn->motion, inn->tiles);
}

int bfm_inn_field_draw(BfmInnField *inn, int show_dialogue, uint32_t pixels[240][320])
{
    if (!inn || !pixels) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x)
            pixels[y][x] = UINT32_C(0xff000000);
    if (!bfm_village_field_prepare(&inn->field, &inn->camera, bfm_bedroom_travel_root(inn->travel),
                                   &inn->pose, inn->record->flags, inn->age, inn->tiles, &pixels[0][0]))
        return 0;
    if (!bfm_inn_keeper_add(inn->keeper, inn->field.hero, bfm_bedroom_field_view(inn->field.field),
                            inn->age)) return 0;
    bfm_bedroom_field_draw(inn->field.field, inn->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, inn->field.hero);
    return !show_dialogue || inn->finished || !inn->greeting ||
           bfm_inn_greeting_draw(inn->greeting, inn->art, inn->tiles, pixels);
}

const BfmForestMessageFrame *bfm_inn_field_window(const BfmInnField *inn)
{
    return inn && inn->updated_greeting ? bfm_inn_greeting_update(inn->greeting) : NULL;
}

void bfm_inn_field_voice_status(BfmInnField *inn, unsigned status)
{
    if (inn) inn->voice_status = status;
}

int bfm_inn_field_service(const BfmInnField *inn)
{
    return inn && inn->greeting && !inn->finished && bfm_inn_greeting_service(inn->greeting);
}

void bfm_inn_field_leave(BfmInnField *inn)
{
    if (bfm_inn_field_service(inn)) inn->finished = 1;
}

int bfm_inn_field_finished(const BfmInnField *inn)
{
    return inn && inn->finished;
}

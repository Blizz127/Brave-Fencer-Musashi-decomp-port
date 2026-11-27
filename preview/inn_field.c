/* SPDX-License-Identifier: MIT */
#include "inn_field.h"
#include "../pc_port/forest_player_bytes.h"
#include "inn_greeting.h"
#include "inn_keeper.h"
#include "village_field.h"
#include "village_clock.h"
#include "forest_message_art.h"
#include "forest_gateway_transition.h"
#include "scene_tiles.h"

#include <stdlib.h>
#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
enum { EXIT_TARGET = 0x260, EXIT_FADE = 21, EXIT_MOUNT = 38 };

typedef struct {
    uint16_t room, resident, event;
    uint32_t outside, face_table;
    size_t descriptor, keeper, walk, inset, room_hp, room_rate;
} InnLayout;

/* The event module is appended to the current chapter's resident field. */
static const InnLayout healthy = {
    .room = 0x2017, .resident = 1, .event = 76, .outside = 0x000a1010, .face_table = 0x8018f338,
    .descriptor = 0x4218, .keeper = 0x40cc, .walk = 0x4100, .inset = 0x65a84,
    .room_hp = 0x9969c, .room_rate = 0x996a8
};
static const InnLayout illness = {
    .room = 0x2058, .resident = 124, .event = 135, .outside = 0x000a1052, .face_table = 0x8018c9dc,
    .descriptor = 0x502c, .keeper = 0x40b4, .walk = 0x40e8, .inset = 0x63128,
    .room_hp = 0x94a04, .room_rate = 0x94a10
};

struct BfmInnField {
    const InnLayout *layout;
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    const uint8_t *module, *outside;
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
    unsigned age, voice_status, exit_age;
    int finished, updated_greeting, paid;
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
    size_t resident = bfm_read_archive_field_scene(disc, "SC03.CD", inn->layout->resident, inn->scene);
    if (resident < inn->layout->room_rate + 6 || resident > sizeof inn->scene - 3) return 0;
    size_t offset = (resident + 3) & ~(size_t)3;
    uint8_t *module = bfm_village_event_data(disc, inn->layout->event, &inn->module_size);
    if (!module) return 0;
    int fits = inn->module_size >= inn->layout->descriptor + 0xd8 &&
               inn->module_size <= sizeof inn->scene - offset;
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
    if (!disc || !transfer || !record || !tiles || transfer[2] ||
        (u16le(transfer) != healthy.room && u32le(transfer) != UINT32_C(0x06002058)))
        return NULL;
    BfmInnField *inn = calloc(1, sizeof *inn);
    if (!inn) return NULL;
    inn->layout = u16le(transfer) == healthy.room ? &healthy : &illness;
    const InnLayout *layout = inn->layout;
    if (!mount_inn(inn, disc)) goto fail;
    const uint8_t *room = inn->module + layout->descriptor;
    uint32_t address = scene_base + (uint32_t)(room - inn->scene);
    const uint8_t *entries = source(inn, room + 0xc8, 3 * 4);
    if (!entries) goto fail;
    const uint8_t *entry = source(inn, entries, 36);
    const uint8_t *outside = source(inn, entries + 2 * 4, 36);
    if (!entry || u32le(entry) != layout->room || !outside || u32le(outside) != layout->outside)
        goto fail;
    inn->outside = outside;
    size_t main_size;
    inn->main = bfm_bedroom_player_data(disc, &main_size);
    if (!inn->main || !bfm_village_field_load_room(&inn->field, disc, inn->scene, inn->scene_size,
                                                   address, layout->face_table)) goto fail;
    inn->motion = bfm_bedroom_motion_open(inn->scene, inn->scene_size, inn->main, main_size, address);
    inn->travel = bfm_bedroom_travel_open(inn->scene, inn->scene_size, address, 0);
    inn->keeper = layout == &illness ? bfm_vambee_inn_keeper_load(inn->scene, inn->scene_size) :
                                       bfm_inn_keeper_load(inn->scene, inn->scene_size);
    if (!inn->motion || !inn->travel || !inn->keeper ||
        !bfm_bedroom_motion_step(inn->motion, BFM_BEDROOM_REST, &inn->pose)) goto fail;
    if (!bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", layout->resident, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", layout->event, tiles) ||
        !bfm_scene_read_palettes(disc, "SC03.CD", layout->event, tiles)) goto fail;
    inn->art = bfm_castle_message_art_load(disc, inn->scene, inn->scene_size, tiles, layout->inset);
    if (!inn->art) goto fail;
    const uint8_t *walk = inn->module + layout->walk;
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

unsigned bfm_inn_field_room(const BfmInnField *inn)
{
    return inn ? inn->layout->room : 0;
}

int bfm_inn_field_step_run(BfmInnField *inn, uint16_t pressed, struct BfmGameplayRun *run)
{
    if (!inn) return 0;
    inn->updated_greeting = 0;
    if (inn->finished || inn->exit_age == EXIT_MOUNT) return 1;
    ++inn->age;
    const BfmBedroomRoot *player = bfm_bedroom_travel_root(inn->travel);
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    BfmBedroomStance stance = BFM_BEDROOM_STANCE_INACTIVE;
    int service = bfm_inn_greeting_service(inn->greeting);
    if (bfm_inn_greeting_leaving(inn->greeting)) ++inn->exit_age;
    if (player->busy) {
        if (!bfm_bedroom_travel_step(inn->travel, inn->camera.eye, inn->camera.target)) return 0;
        switch (player->locomotion_edge) {
        case BFM_TRAVEL_WALK: action = BFM_BEDROOM_WALK; break;
        case BFM_TRAVEL_STAND:
            action = BFM_BEDROOM_REST;
            if (!bfm_inn_greeting_leaving(inn->greeting))
                bfm_bedroom_travel_face_point(inn->travel, s16le(inn->module + inn->layout->keeper),
                                              s16le(inn->module + inn->layout->keeper + 4));
            break;
        case BFM_TRAVEL_FALL: action = BFM_BEDROOM_FALL; break;
        case BFM_TRAVEL_LAND: action = BFM_BEDROOM_LAND; break;
        default: break;
        }
    } else if (!inn->greeting) {
        stance = BFM_BEDROOM_STANCE_SCRIPTED;
        size_t resident_bytes = (size_t)(inn->module - inn->scene);
        if (inn->layout == &illness)
            inn->greeting = bfm_inn_greeting_open_illness(inn->module, inn->module_size,
                                                          inn->scene, resident_bytes, inn->record);
        else
            inn->greeting = bfm_inn_greeting_open(inn->module, inn->module_size,
                                                  inn->scene, resident_bytes, inn->record);
        if (!inn->greeting) return 0;
    } else if (!bfm_inn_greeting_leaving(inn->greeting)) {
        stance = BFM_BEDROOM_STANCE_SCRIPTED;
        if (!bfm_inn_greeting_step(inn->greeting, pressed, inn->voice_status)) return 0;
        inn->updated_greeting = 1;
        if (bfm_inn_greeting_leaving(inn->greeting)) {
            /* The farewell starts a scripted walk from the counter to the
             * interior door. The exterior transfer runs while Musashi walks. */
            const uint8_t *door = inn->module + EXIT_TARGET;
            if (!bfm_bedroom_travel_walk(inn->travel, s16le(door), s16le(door + 4), 0))
                return 0;
        }
        if (bfm_inn_greeting_finished(inn->greeting)) inn->finished = 1;
    }
    /* The service and room-selection panels hold the counter view. The
     * follow-up conversation resumes Musashi's standing pose at the counter. */
    if (run && service) return 1;
    return bfm_bedroom_motion_step_field(inn->motion, action, run, stance, &inn->pose) &&
           bfm_bedroom_motion_face(inn->motion, inn->tiles);
}

int bfm_inn_field_step(BfmInnField *inn, uint16_t pressed)
{
    return bfm_inn_field_step_run(inn,pressed,NULL);
}

int bfm_inn_field_draw(BfmInnField *inn, int dialogue, uint32_t pixels[240][320])
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
    return !dialogue || inn->finished || !inn->greeting ||
           bfm_inn_greeting_draw(inn->greeting, dialogue, inn->art, inn->tiles, pixels);
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

const uint16_t *bfm_inn_field_menu_palette(const BfmInnField *inn)
{
    return inn ? &inn->tiles[257][352] : NULL;
}

unsigned bfm_inn_room_price(const BfmInnField *inn, unsigned room)
{
    /* The last two rooms are occupied on this early village visit. */
    if (!inn || room >= 4) return 0;
    unsigned rate = inn->scene[inn->layout->room_rate + room];
    return rate > 127 ? 10 * (256 - rate) : 0;
}

static void put_word(uint8_t *out, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        out[i] = (uint8_t)(value >> (i * 8));
}

int bfm_inn_pay_room201(BfmInnField *inn)
{
    if (!bfm_inn_field_service(inn) || inn->paid) return 0;
    uint8_t *record = inn->record->progress;
    /* The wallet holds tens of Drans. An unaffordable stay changes nothing. */
    unsigned charge = bfm_inn_room_price(inn, 0) / 10;
    uint32_t wallet = u32le(record + 0x14);
    if (!charge || charge > wallet) return 0;
    put_word(record + 0x14, wallet - charge);
    inn->paid = 1;
    return 1;
}

static void restore_points(uint8_t *vital, unsigned amount)
{
    unsigned maximum = u16le(vital);
    unsigned total = u16le(vital + 2) + amount;
    if (total > maximum) total = maximum;
    vital[2] = (uint8_t)total;
    vital[3] = (uint8_t)(total >> 8);
}

void bfm_inn_finish_room201(BfmInnField *inn)
{
    if (!inn || !inn->paid) return;
    inn->paid = 0;
    uint8_t *p = inn->record->progress;
    restore_points(p + 0x3a, u16le(inn->scene + inn->layout->room_hp));
    restore_points(p + 0x3e, 999);
    /* Both fixed-point stamina reserves reach their rested limit. */
    put_word(p + 0x2c, 160u << 16);
    put_word(p + 0x60, 160u << 16);
    bfm_village_clock_rest(inn->record, 8);
}

int bfm_inn_field_leave(BfmInnField *inn)
{
    return bfm_inn_field_service(inn) && bfm_inn_greeting_finish_service(inn->greeting);
}

void bfm_inn_field_shade(const BfmInnField *inn, uint32_t pixels[240][320])
{
    if (!inn || inn->exit_age <= EXIT_FADE) return;
    /* The exit darkens the room and the overlaid HUD uniformly. */
    BfmForestGatewayFrame fade = {.shade = 2 * (inn->exit_age - EXIT_FADE)};
    bfm_forest_gateway_transition_shade(&fade, pixels);
}

const uint8_t *bfm_inn_field_departure(const BfmInnField *inn)
{
    return inn && inn->exit_age == EXIT_MOUNT ? inn->outside : NULL;
}

int bfm_inn_field_finished(const BfmInnField *inn)
{
    return inn && inn->finished;
}

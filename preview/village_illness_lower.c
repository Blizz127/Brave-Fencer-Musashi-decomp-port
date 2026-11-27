/* SPDX-License-Identifier: MIT */
#include "village_illness_lower.h"
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

static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t street_room = UINT32_C(0x80190780);
enum { STREET_MEMBER = 124, INN_ENTRANCE = 10, REVEAL_FIELDS = 1 + 16 };

struct BfmVillageIllnessLower {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    BfmSaveRecord *save;
    uint16_t (*tiles)[1024];
    BfmVillageField field;
    BfmVillageMotion *motion;
    BfmVillagePlayer player;
    BfmVillageCamera camera;
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

static int mount_camera(BfmVillageIllnessLower *street)
{
    const uint8_t *room = mounted(street, street_room, 0xcc);
    const uint8_t *entries = room ? mounted(street, u32le(room + 0xc8), 4 * (INN_ENTRANCE + 1)) : NULL;
    const uint8_t *inn = entries ? mounted(street, u32le(entries + 4 * INN_ENTRANCE), 36) : NULL;
    const uint8_t *angles = mounted(street, UINT32_C(0x8018aecc), 8 * 2);
    if (!inn || inn[3] >= 8 || !angles) return 0;
    unsigned yaw = u16le(angles + 2 * inn[3]);
    if (yaw >= 4096) return 0;
    bfm_village_camera_lower(&street->camera, street->player.root.xyz, 1);
    street->turn.goal = street->camera.yaw = (int)yaw;
    bfm_village_camera_lower(&street->camera, street->player.root.xyz, 0);
    return 1;
}

void bfm_village_illness_lower_free(BfmVillageIllnessLower *street)
{
    if (!street) return;
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
    if (!disc || !entry || !main || !tiles || !hud || entry->scene_member != STREET_MEMBER ||
        u32le(entry->state.destination) != UINT32_C(0x000a1052)) return NULL;
    BfmVillageIllnessLower *street = calloc(1, sizeof *street);
    if (!street) return NULL;
    street->tiles = tiles;
    street->save = &entry->state;
    street->bytes = bfm_read_archive_field_scene(disc, "SC03.CD", STREET_MEMBER, street->scene);
    street->motion = bfm_village_motion_resume(street->scene, street->bytes, street_room, main, bytes);
    if (!street->motion) goto fail;
    street->player = *bfm_village_motion_player(street->motion);
    street->player.controlled = 0;
    street->reveal_fields = REVEAL_FIELDS;
    if (!mount_camera(street) ||
        !bfm_village_field_load_room(&street->field, disc, street->scene, street->bytes, street_room,
                                     UINT32_C(0x8018c9dc))) goto fail;
    street->scenery = bfm_village_street_scenery_load(street->scene, street->bytes, street_room);
    street->doors = bfm_village_shop_doors_load(street->scene, street->bytes, street_room);
    if (!street->scenery || !street->doors ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", STREET_MEMBER, tiles) ||
        !bfm_village_illness_light_begin(&street->daylight, street->scene, street->bytes,
                                        street->save->progress[0x37], tiles) ||
        !bfm_village_motion_lower_orb(street->motion, street->save->progress[0x48], tiles))
        goto fail;
    BfmForestHud *mounted_hud = bfm_village_illness_hud_load(disc, street->scene, street->bytes, tiles);
    if (!mounted_hud) goto fail;
    bfm_village_street_scenery_step(street->scenery, 0, u32le(street->save->progress + 8));
    *hud = mounted_hud;
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

int bfm_village_illness_lower_step(BfmVillageIllnessLower *street, BfmGameplayRun *run,
                                  BfmFieldInput input, unsigned scene_tick)
{
    if (!street) return 0;
    int entering = street->reveal_fields != 0;
    int departing = bfm_village_illness_lower_door(street) != NULL;
    int asleep = 0;
    if (!departing) {
        /* The receiving operation services the ordinary standing actor with
         * a neutral pad through its final reveal field. */
        if (entering) {
            input = (BfmFieldInput){0};
            --street->reveal_fields;
        } else {
            bfm_field_rest_input(&street->rest, street->save, input);
        }
        bfm_village_motion_woods_condition(street->motion, bfm_field_tired_stance(street->save->progress));
        asleep = bfm_field_rest_sleeping(&street->rest);
        if (street->rest.pose == BFM_FIELD_REST_FREE) {
            int bearing = (street->camera.yaw + 2048) & 4095;
            if (!bfm_village_motion_step_field(street->motion, input.x, input.y, bearing, run)) return 0;
        } else {
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

    int turn = street->player.controlled && street->rest.pose == BFM_FIELD_REST_FREE ? input.camera_turn : 0;
    bfm_village_camera_turn(&street->camera, &street->turn, turn);
    bfm_village_camera_lower(&street->camera, street->player.root.xyz, 0);
    bfm_village_daylight_step(&street->daylight, street->save->progress[0x37], street->tiles);
    if (!entering && !departing) {
        bfm_field_rest_stamina(street->save, asleep);
        bfm_village_clock_step(street->save, asleep);
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
        !bfm_village_field_prepare(&street->field, &street->camera, &street->player.root,
                                   &street->player.pose, flags, scene_tick, street->tiles, &pixels[0][0]))
        return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(street->field.field);
    bfm_village_street_scenery_lights(street->scenery, street->daylight.lamps);
    bfm_village_shop_doors_prepare(street->doors, view, street->tiles, &pixels[0][0], street->save);
    if (!bfm_village_street_scenery_prepare(street->scenery, view, street->tiles, &pixels[0][0], draw_page) ||
        !bfm_village_street_vane_add(street->scenery, street->field.hero, view)) return 0;
    bfm_bedroom_field_draw(street->field.field, street->tiles, &pixels[0][0], draw_to, street);
    return 1;
}

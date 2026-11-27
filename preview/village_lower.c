/* SPDX-License-Identifier: MIT */
#include "village_lower.h"
#include "village_field.h"
#include "village_clock.h"
#include "village_lighting.h"
#include "../pc_port/forest_player_bytes.h"
#include "village_mayor.h"
#include "village_mayor_stage.h"
#include "village_mayor_door.h"
#include "village_portrait.h"
#include "village_residents.h"
#include "village_sky.h"
#include "village_street_scenery.h"
#include "forest_message_art.h"
#include "scene_tiles.h"

#include <limits.h>
#include <stdlib.h>

static const uint32_t room_root = UINT32_C(0x80192b18);
static const uint32_t scene_origin = UINT32_C(0x80128158);

struct BfmVillageLower {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t scene_bytes, module_bytes;
    uint8_t *module;
    uint16_t (*tiles)[1024];
    BfmVillageMotion *motion;
    BfmVillageField field;
    BfmVillageCamera camera, presented;
    BfmVillageCameraTurn turn;
    BfmVillageMayorStage *stage;
    BfmVillageStreetScenery *scenery;
    BfmVillageShopDoors *shop_doors;
    BfmVillageMayorDialogue talk;
    BfmSaveRecord *save;
    BfmForestMessageArt *art;
    unsigned age;
    int latch, anchor, right;
};

void bfm_village_lower_free(BfmVillageLower *lower)
{
    if (!lower) return;
    bfm_forest_message_art_free(lower->art);
    bfm_village_mayor_stage_free(lower->stage);
    bfm_village_shop_doors_free(lower->shop_doors);
    bfm_village_street_scenery_free(lower->scenery);
    bfm_village_motion_free(lower->motion);
    bfm_village_field_clear(&lower->field);
    free(lower->module);
    free(lower);
}

static BfmVillageLower *lower_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                   const BfmVillageMotion *departed, const uint8_t *main,
                                   size_t main_bytes, uint16_t tiles[512][1024], BfmForestHud **hud)
{
    BfmBedroomEntry next = *entry;
    if (departed && !bfm_castle_room_entry(disc, bfm_village_motion_transfer(departed), &next))
        return NULL;
    if (next.scene_member != 1) return NULL;
    BfmVillageLower *lower = calloc(1, sizeof *lower);
    if (!lower) return NULL;
    lower->scene_bytes = bfm_read_archive_field_scene(disc, "SC03.CD", next.scene_member, lower->scene);
    if (lower->scene_bytes < room_root - scene_origin + 0xcc) goto fail;
    lower->motion = departed ? bfm_village_motion_arrive(departed, lower->scene, lower->scene_bytes, room_root) :
                    bfm_village_motion_resume(lower->scene, lower->scene_bytes, room_root, main, main_bytes);
    if (!lower->motion || !bfm_village_field_load_lower(&lower->field, disc, lower->scene,
                                                       lower->scene_bytes, room_root)) goto fail;
    lower->module = bfm_village_event_data(disc, 53, &lower->module_bytes);
    if (!lower->module) goto fail;
    lower->stage = bfm_village_mayor_stage_open(lower->scene, lower->scene_bytes, room_root,
                        lower->module, lower->module_bytes, UINT32_C(0x80190fb0));
    lower->shop_doors = bfm_village_shop_doors_load(lower->scene, lower->scene_bytes, room_root);
    lower->scenery = bfm_village_street_scenery_load(lower->scene, lower->scene_bytes, room_root);
    if (!lower->scenery || !lower->stage || !lower->shop_doors ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", next.scene_member, tiles)) goto fail;
    bfm_village_mayor_portrait_load(tiles);
    lower->art = bfm_castle_message_art_load(disc, lower->scene, lower->scene_bytes, tiles, 0x65a84);
    if (!lower->art) goto fail;
    lower->tiles = tiles;
    lower->save = &entry->state;
    const BfmVillagePlayer *p = bfm_village_motion_player(lower->motion);
    bfm_village_camera_lower(&lower->camera, p->root.xyz, 1);
    if (hud) {
        /* The exterior cold view faces back toward the inn entrance. */
        lower->camera.yaw = 3584;
        bfm_village_camera_lower(&lower->camera, p->root.xyz, 0);
        if (!bfm_village_motion_lower_orb(lower->motion, next.state.progress[0x48], tiles))
            goto fail;
        *hud = bfm_village_hud_load(disc, lower->scene, lower->scene_bytes, tiles);
        if (!*hud) goto fail;
    }
    lower->turn.goal = lower->camera.yaw;
    lower->presented = lower->camera;
    *entry = next;
    return lower;
fail:
    bfm_village_lower_free(lower);
    return NULL;
}

BfmVillageLower *bfm_village_lower_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                       const BfmVillageMotion *departed, uint16_t tiles[512][1024])
{
    if (!departed) return NULL;
    return lower_open(disc, entry, departed, NULL, 0, tiles, NULL);
}

BfmVillageLower *bfm_village_lower_resume(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                         const uint8_t *main, size_t bytes,
                                         uint16_t tiles[512][1024], BfmForestHud **hud)
{
    if (!hud || entry->state.destination[2] != 10) return NULL;
    return lower_open(disc, entry, NULL, main, bytes, tiles, hud);
}

static const uint8_t *scene_pointer(const BfmVillageLower *lower, const uint8_t *slot,
                                    size_t length)
{
    uint32_t address = (uint32_t)slot[0] | (uint32_t)slot[1] << 8 |
                       (uint32_t)slot[2] << 16 | (uint32_t)slot[3] << 24;
    if (address < scene_origin || length > lower->scene_bytes ||
        address - scene_origin > lower->scene_bytes - length) return NULL;
    return lower->scene + (address - scene_origin);
}

const uint8_t *bfm_village_lower_inn_transfer(const BfmVillageLower *lower)
{
    if (!lower || bfm_village_mayor_stage_active(lower->stage)) return NULL;
    const BfmVillagePlayer *player = bfm_village_motion_player(lower->motion);
    if (!player->controlled || (player->wall_contact & 31u) != 14) return NULL;
    /* Lower village wall 14 leads through descriptor 10. It also carries
     * the inn's exterior return position and heading. */
    const uint8_t *room = lower->scene + (room_root - scene_origin);
    const uint8_t *entries = scene_pointer(lower, room + 0xc8, 11 * 4);
    const uint8_t *door = entries ? scene_pointer(lower, entries + 10 * 4, 36) : NULL;
    if (!door || door[0] != 0x17 || door[1] != 0x20 || door[2] != 0) return NULL;
    return door;
}

const BfmVillageMotion *bfm_village_lower_departure(const BfmVillageLower *lower)
{
    if (!lower || bfm_village_mayor_stage_active(lower->stage)) return NULL;
    return bfm_village_motion_transfer(lower->motion) ? lower->motion : NULL;
}

const BfmVillagePlayer *bfm_village_lower_player(const BfmVillageLower *lower)
{
    return bfm_village_motion_player(lower->motion);
}

void bfm_village_lower_close(BfmVillageLower *lower, unsigned answer)
{
    bfm_village_mayor_dialogue_close(&lower->talk, answer);
    if (lower->talk.finished) bfm_village_mayor_stage_finish(lower->stage);
}

int bfm_village_lower_step_field(BfmVillageLower *lower, BfmFieldInput field,
                                BfmForestMessage **opened)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(lower->motion);
    int active = bfm_village_mayor_stage_active(lower->stage);
    if (!active && p->controlled && field.square_pressed &&
        bfm_village_mayor_stage_begin(lower->stage, lower->save, &p->root, &lower->camera, field.square_pressed)) {
        lower->talk = (BfmVillageMayorDialogue){0};
        active = 1;
    }
    if (!active && !bfm_village_motion_step(lower->motion, field.x, field.y,
                                (lower->camera.yaw + 2048) & 4095)) return 0;
    if (!active && p->controlled && !(p->events & BFM_VILLAGE_TRANSFER))
        bfm_village_camera_turn(&lower->camera, &lower->turn, field.camera_turn);
    else
        lower->turn.direction = lower->turn.held_ticks = 0;
    bfm_village_camera_lower(&lower->camera, p->root.xyz, 0);
    lower->presented = lower->camera;
    if (active) {
        if (!bfm_village_mayor_stage_step(lower->stage, &lower->presented)) return 0;
        if (!bfm_village_mayor_stage_active(lower->stage)) {
            if (!bfm_village_motion_place(lower->motion, bfm_village_mayor_stage_player(lower->stage)))
                return 0;
        } else if (!bfm_village_motion_porch(lower->motion, bfm_village_mayor_stage_player(lower->stage),
                                            bfm_village_mayor_stage_knock(lower->stage))) {
            return 0;
        }
        if (bfm_village_mayor_stage_message_ready(lower->stage) && !lower->talk.finished) {
            if (!lower->talk.cursor && !bfm_village_mayor_dialogue_begin(&lower->talk, lower->module,
                    lower->module_bytes, lower->save)) return 0;
            if (!bfm_village_mayor_dialogue_step(&lower->talk, lower->save, opened)) return 0;
            if (*opened) lower->latch = 1;
            if (lower->talk.finished) bfm_village_mayor_stage_finish(lower->stage);
        }
    }
    if (!active && !(p->events & BFM_VILLAGE_TRANSFER)) bfm_village_clock_step(lower->save, 0);
    ++lower->age;
    return bfm_village_motion_face(lower->motion, lower->tiles);
}

int bfm_village_lower_step(BfmVillageLower *lower, int x, int y, int square,
                          BfmForestMessage **opened)
{
    BfmFieldInput field = {.x = x, .y = y, .square_pressed = square};
    return bfm_village_lower_step_field(lower, field, opened);
}

static void draw_figures(void *context, int depth)
{
    BfmVillageLower *lower = context;
    for (;;) {
        int tree = bfm_village_street_scenery_next_depth(lower->scenery);
        int actor = bfm_bedroom_figure_next_depth(lower->field.hero);
        int door = bfm_village_mayor_stage_next_depth(lower->stage);
        int bucket = tree > actor ? tree : actor;
        if (door > bucket) bucket = door;
        int shop = bfm_village_shop_doors_next_depth(lower->shop_doors);
        if (shop > bucket) bucket = shop;
        if (bucket == INT_MIN || bucket < depth) break;
        bfm_village_shop_doors_draw_to(lower->shop_doors, bucket);
        bfm_village_mayor_stage_draw_to(lower->stage, bucket);
        bfm_bedroom_figure_draw_to(lower->field.hero, bucket);
        bfm_village_street_scenery_draw_to(lower->scenery, bucket);
    }
    if (depth == INT_MIN) bfm_bedroom_figure_draw_to(lower->field.hero, depth);
}

int bfm_village_lower_draw(BfmVillageLower *lower, unsigned tick, const uint8_t flags[64],
                          const BfmForestMessageFrame *window, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_village_motion_player(lower->motion);
    const BfmBedroomRoot *root = bfm_village_mayor_stage_active(lower->stage)
                                  ? bfm_village_mayor_stage_player(lower->stage) : &p->root;
    unsigned minutes = lower->save->progress[0x1a] | (unsigned)lower->save->progress[0x1b] << 8;
    if (!bfm_village_lower_light(lower->field.hero, lower->scene, lower->scene_bytes,
                                lower->save->progress[0x37]) ||
        !bfm_lower_village_sky_draw(lower->scene, lower->scene_bytes, minutes / 60, pixels) ||
        !bfm_village_field_prepare(&lower->field, &lower->presented, root, &p->pose, flags,
                                   lower->age, lower->tiles, &pixels[0][0])) return 0;
    bfm_village_street_scenery_step(lower->scenery, tick, u32le(lower->save->progress + 8));
    const BfmBedroomView *camera = bfm_bedroom_field_view(lower->field.field);
    bfm_village_shop_doors_prepare(lower->shop_doors, camera, lower->tiles, &pixels[0][0], lower->save);
    if (!bfm_village_mayor_stage_prepare(lower->stage, camera, lower->tiles, &pixels[0][0]) ||
        !bfm_village_street_scenery_prepare(lower->scenery, camera, lower->tiles, &pixels[0][0]) ||
        !bfm_village_mayor_stage_add(lower->stage, lower->field.hero, camera)) return 0;
    bfm_bedroom_field_draw(lower->field.field, lower->tiles, &pixels[0][0], draw_figures, lower);
    if (window) {
        if (lower->latch) {
            const int16_t *mayor = bfm_village_mayor_stage_position(lower->stage);
            const int16_t *speaker = lower->talk.hero ? root->xyz : mayor;
            const int16_t *listener = lower->talk.hero ? mayor : root->xyz;
            const uint8_t *limits = lower->scene + (UINT32_C(0x8018dc54) - scene_origin);
            if (!bfm_village_speaker_anchor(camera, speaker, listener, limits,
                                           &lower->anchor, &lower->right)) return 0;
            lower->latch = 0;
        }
        BfmForestMessageFrame shown = *window;
        if (lower->age & 4u) shown.inline_prompt = 0;
        bfm_bedroom_message_art_draw_side(lower->art, &shown, lower->anchor, lower->right, pixels);
    }
    return 1;
}

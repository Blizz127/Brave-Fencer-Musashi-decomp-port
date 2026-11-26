/* SPDX-License-Identifier: MIT */
#include "village_arrival.h"
#include "forest_message_art.h"
#include "scene_tiles.h"
#include "village_field.h"
#include "village_clock.h"
#include "village_lower.h"
#include "village_cable.h"
#include "village_impact.h"
#include "village_portrait.h"
#include "village_greeting.h"
#include "village_gate.h"
#include "village_residents.h"
#include "village_sky.h"
#include "village_talk_camera.h"
#include <limits.h>
#include <stdlib.h>

struct BfmVillageArrival {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes, descriptor;
    const MusashiDiscMedia *disc;
    BfmBedroomEntry *entry;
    BfmVillageLower *lower;
    unsigned transfer_tick;
    uint16_t (*tiles)[1024];
    BfmVillageMotion *motion;
    BfmVillageCamera camera, presented_camera;
    BfmVillageTalkCamera talk_camera;
    BfmSaveRecord *record;
    int start_talk_camera, latch_window, resident_anchor, resident_right;
    BfmVillageField view;
    BfmVillageResidents *residents;
    BfmVillageGate *gate;
    BfmVillageCable *cable;
    BfmVillageImpact *impact;
    BfmForestDustCard cards[BFM_VILLAGE_IMPACT_CARDS];
    unsigned next_card, card_count;
    BfmForestRaster surface;
    BfmForestMessageArt *message_art;
    unsigned age;
};

void bfm_village_arrival_free(BfmVillageArrival *a)
{
    if (!a) return;
    bfm_village_lower_free(a->lower);
    bfm_forest_message_art_free(a->message_art);
    bfm_village_residents_free(a->residents);
    bfm_village_impact_free(a->impact);
    bfm_village_gate_free(a->gate);
    bfm_village_cable_free(a->cable);
    bfm_village_motion_free(a->motion);
    bfm_village_field_clear(&a->view);
    free(a);
}

BfmVillageArrival *bfm_village_arrival_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                      const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    BfmBedroomEntry next = *entry;
    if (!bfm_castle_room_entry(disc, entry->state.destination, &next) || next.scene_member != 2)
        return NULL;
    BfmVillageArrival *a = calloc(1, sizeof *a);
    if (!a) return NULL;
    a->bytes = bfm_read_archive_field_scene(disc, "SC03.CD", next.scene_member, a->scene);
    const uint32_t room = 0x8018c794u;
    a->descriptor = room - 0x80128158u;
    if (a->descriptor > a->bytes || a->bytes - a->descriptor < 0xcc) goto fail;
    a->motion = bfm_village_motion_open(a->scene, a->bytes, main, main_bytes, room, 5);
    if (!a->motion || !bfm_village_field_load(&a->view, disc, a->scene, a->bytes, room) ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", next.scene_member, tiles)) goto fail;
    bfm_village_macho_portrait_load(tiles);
    a->message_art = bfm_castle_message_art_load(disc, a->scene, a->bytes, tiles, 0x5da74);
    a->gate = bfm_village_gate_load(a->scene, a->bytes, room, next.state.progress[8], next.state.flags);
    a->impact = bfm_village_impact_open(a->scene, a->bytes, main, main_bytes);
    a->cable = bfm_village_cable_load(a->scene, a->bytes, room);
    if (!a->message_art || !a->gate || !a->impact || !a->cable) goto fail;
    if (!(next.state.flags[240 / 8] & 1u) && next.state.progress[8] == 10) {
        a->residents = bfm_village_residents_load(a->scene, a->bytes, room, 0x80188094u);
        if (!a->residents) goto fail;
    }
    a->tiles = tiles;
    const int16_t *spawn = bfm_village_motion_player(a->motion)->root.xyz;
    bfm_village_camera_begin(&a->camera, spawn);
    /* The first live camera update precedes Musashi's first moving update. */
    bfm_village_camera_step(&a->camera, spawn, 0);
    a->presented_camera = a->camera;
    *entry = next;
    a->disc = disc;
    a->entry = entry;
    a->record = &entry->state;
    return a;
fail:
    bfm_village_arrival_free(a);
    return NULL;
}

const BfmVillagePlayer *bfm_village_arrival_player(const BfmVillageArrival *a)
{
    return a->lower ? bfm_village_lower_player(a->lower) : bfm_village_motion_player(a->motion);
}

int bfm_village_arrival_continue(BfmVillageArrival *a)
{
    if (bfm_village_residents_talking(a->residents)) {
        bfm_village_residents_close_talk(a->residents);
        return 1;
    }
    return bfm_village_motion_continue(a->motion);
}

int bfm_village_arrival_message_closed(BfmVillageArrival *a, unsigned answer)
{
    if (a->lower) {
        bfm_village_lower_close(a->lower, answer);
        return 1;
    }
    return bfm_village_arrival_continue(a);
}

unsigned bfm_village_arrival_transfer_tick(const BfmVillageArrival *a)
{
    return a->transfer_tick;
}

BfmForestMessage *bfm_village_arrival_speech(const BfmVillageArrival *a)
{
    BfmVillagePhase phase = bfm_village_arrival_player(a)->phase;
    if (phase != BFM_VILLAGE_FIRST_MESSAGE && phase != BFM_VILLAGE_SECOND_MESSAGE) return NULL;
    return bfm_village_greeting_open(a->scene, a->bytes,
                phase == BFM_VILLAGE_FIRST_MESSAGE ? BFM_VILLAGE_ARRIVAL : BFM_VILLAGE_HUNGER);
}

int bfm_village_arrival_step_input(BfmVillageArrival *a, BfmGameplayRun *run,
                                   int x, int y, int talk_edge, BfmForestMessage **opened)
{
    if (opened) *opened = NULL;
    if (a->transfer_tick) {
        /* The outgoing field freezes at its exit contact. The incoming mesh is
         * ready before the crossfade makes its first scanout visible. */
        if (++a->transfer_tick == 145) {
            a->lower = bfm_village_lower_open(a->disc, a->entry, a->motion, a->tiles);
            if (!a->lower) return 0;
        }
        if (a->transfer_tick < 161) return 1;
        if (a->transfer_tick >= 166) a->transfer_tick = 0;
    }
    if (a->lower) return bfm_village_lower_step(a->lower, x, y, talk_edge, opened);
    const BfmVillagePlayer *before = bfm_village_motion_player(a->motion);
    if (opened && before->controlled && talk_edge &&
        bfm_village_residents_begin_talk(a->residents, before->root.xyz, a->record, talk_edge)) {
        int16_t station[3];
        unsigned yaw;
        if (!bfm_village_residents_station(a->residents, station, &yaw) ||
            !bfm_village_motion_begin_talk(a->motion, station, yaw)) return 0;
        a->start_talk_camera = 1;
    } else if (a->start_talk_camera) {
        if (!bfm_village_talk_camera_begin(&a->talk_camera, a->scene, a->bytes,
                                           &a->presented_camera)) return 0;
        a->start_talk_camera = 0;
    }
    /* The camera's orbit yaw points from the target to the eye. */
    int movement_yaw = (a->camera.yaw + 2048) & 4095;
    if (a->talk_camera.phase)
        movement_yaw = bfm_village_motion_view_yaw(a->presented_camera.eye, a->presented_camera.target);
    if (!bfm_village_motion_step(a->motion, x, y, movement_yaw)) return 0;
    const BfmVillagePlayer *p = bfm_village_motion_player(a->motion);
    if (p->events & BFM_VILLAGE_TRANSFER) a->transfer_tick = 1;
    if (p->controlled && !a->transfer_tick && !bfm_village_residents_talking(a->residents))
        bfm_village_clock_step(a->record, 0);
    bfm_village_cable_step(a->cable, p->movement, p->movement <= 81, p->root.xyz, run);
    bfm_village_impact_step(a->impact, p->events & BFM_VILLAGE_HIT, run);
    bfm_village_camera_step(&a->camera, p->root.xyz, p->events & BFM_VILLAGE_CAMERA);
    if (bfm_village_residents_talking(a->residents)) {
        int returning = bfm_village_residents_camera_returning(a->residents);
        if (!opened || !bfm_village_residents_step_talk(a->residents, a->record, a->age, opened))
            return 0;
        if (*opened) a->latch_window = 1;
        if (!returning && bfm_village_residents_camera_returning(a->residents))
            bfm_village_talk_camera_release(&a->talk_camera);
        if (!bfm_village_residents_talking(a->residents)) {
            if (!bfm_village_motion_end_talk(a->motion)) return 0;
        }
    }
    a->presented_camera = a->camera;
    bfm_village_talk_camera_step(&a->talk_camera, &a->presented_camera);
    ++a->age;
    return bfm_village_motion_face(a->motion, a->tiles);
}

int bfm_village_arrival_step(BfmVillageArrival *a, BfmGameplayRun *run, int x, int y)
{
    return bfm_village_arrival_step_input(a, run, x, y, 0, NULL);
}

static int far_card_first(const void *left, const void *right)
{
    const BfmForestDustCard *a = left, *b = right;
    if (a->bucket != b->bucket) return a->bucket < b->bucket ? 1 : -1;
    return (int)b->slot - (int)a->slot;
}

static void draw_actors(void *context, int depth)
{
    BfmVillageArrival *a = context;
    for (;;) {
        int bucket = bfm_village_cable_next_depth(a->cable);
        int gate = bfm_village_gate_next_depth(a->gate);
        int hero = bfm_bedroom_figure_next_depth(a->view.hero);
        if (gate > bucket) bucket = gate;
        if (hero > bucket) bucket = hero;
        if (a->next_card < a->card_count && (int)a->cards[a->next_card].bucket > bucket)
            bucket = (int)a->cards[a->next_card].bucket;
        if (bucket == INT_MIN || bucket < depth) break;

        while (a->next_card < a->card_count && (int)a->cards[a->next_card].bucket == bucket) {
            const BfmForestDustCard *card = &a->cards[a->next_card++];
            bfm_forest_raster_face(&a->surface, &card->material, card->vertices);
        }
        bfm_village_gate_draw_to(a->gate, bucket);
        bfm_bedroom_figure_draw_to(a->view.hero, bucket);
        bfm_village_cable_draw_to(a->cable, bucket);
    }
    if (depth == INT_MIN) {
        bfm_village_gate_draw_to(a->gate, depth);
        bfm_bedroom_figure_draw_to(a->view.hero, depth);
    }
}

int bfm_village_arrival_draw(BfmVillageArrival *a, const uint8_t flags[64],
                             const BfmForestMessageFrame *window, uint32_t pixels[240][320])
{
    if (a->lower) return bfm_village_lower_draw(a->lower, flags, window, pixels);
    const BfmVillagePlayer *p = bfm_village_motion_player(a->motion);
    BfmVillageCamera drawing = a->presented_camera;
    int shake = bfm_village_impact_shake_y(a->impact);
    drawing.eye[1] += shake;
    drawing.target[1] += shake;
    if (!bfm_village_sky_draw(a->scene + a->descriptor, a->bytes - a->descriptor, pixels) ||
        !bfm_village_field_prepare(&a->view, &drawing, &p->root, &p->pose, flags, a->age,
                                   a->tiles, &pixels[0][0])) return 0;
    const BfmBedroomView *camera = bfm_bedroom_field_view(a->view.field);
    if (!bfm_village_cable_prepare(a->cable, camera, p->root.xyz, p->phase == BFM_VILLAGE_RIDE,
                                  a->tiles, &pixels[0][0])) return 0;
    if (!bfm_village_gate_prepare(a->gate, camera, a->tiles, &pixels[0][0])) return 0;
    if (a->residents && !bfm_village_residents_add(a->residents, a->view.hero, camera, a->age))
        return 0;
    a->surface = (BfmForestRaster){.tiles=a->tiles, .pixels=pixels, .top=10, .bottom=229};
    a->next_card = 0;
    a->card_count = bfm_village_impact_project(a->impact, camera, a->cards);
    qsort(a->cards, a->card_count, sizeof *a->cards, far_card_first);
    bfm_bedroom_field_draw(a->view.field, a->tiles, &pixels[0][0],
                           draw_actors, a);
    if (window && bfm_village_residents_talking(a->residents)) {
        if (a->latch_window) {
            if (!bfm_village_residents_place_window(a->residents, camera, p->root.xyz,
                                                    &a->resident_anchor, &a->resident_right)) return 0;
            a->latch_window = 0;
        }
        BfmForestMessageFrame visible = *window;
        if (a->age & 4u) visible.inline_prompt = 0;
        bfm_bedroom_message_art_draw_side(a->message_art, &visible, a->resident_anchor,
                                          a->resident_right, pixels);
    } else if (window) {
        int point[3];
        if (!bfm_bedroom_project(camera, p->root.xyz, point)) return 0;
        bfm_bedroom_message_art_draw_side(a->message_art, window, point[0], 0, pixels);
    }
    return 1;
}

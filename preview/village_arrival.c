/* SPDX-License-Identifier: MIT */
#include "village_arrival.h"
#include "forest_message_art.h"
#include "scene_tiles.h"
#include "village_field.h"
#include "village_impact.h"
#include "village_greeting.h"
#include "village_gate.h"
#include "village_residents.h"
#include "village_sky.h"
#include <stdlib.h>

struct BfmVillageArrival {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes, descriptor;
    uint16_t (*tiles)[1024];
    BfmVillageMotion *motion;
    BfmVillageCamera camera;
    BfmVillageField view;
    BfmVillageResidents *residents;
    BfmVillageGate *gate;
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
    bfm_forest_message_art_free(a->message_art);
    bfm_village_residents_free(a->residents);
    bfm_village_impact_free(a->impact);
    bfm_village_gate_free(a->gate);
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
    a->message_art = bfm_castle_message_art_load(disc, a->scene, a->bytes, tiles, 0x5da74);
    a->gate = bfm_village_gate_load(a->scene, a->bytes, room, next.state.progress[8], next.state.flags);
    a->impact = bfm_village_impact_open(a->scene, a->bytes, main, main_bytes);
    if (!a->message_art || !a->gate || !a->impact) goto fail;
    if (!(next.state.flags[240 / 8] & 1u) && next.state.progress[8] == 10) {
        a->residents = bfm_village_residents_load(a->scene, a->bytes, room, 0x80188094u);
        if (!a->residents) goto fail;
    }
    a->tiles = tiles;
    bfm_village_camera_begin(&a->camera, bfm_village_motion_player(a->motion)->root.xyz);
    *entry = next;
    return a;
fail:
    bfm_village_arrival_free(a);
    return NULL;
}

const BfmVillagePlayer *bfm_village_arrival_player(const BfmVillageArrival *a)
{
    return bfm_village_motion_player(a->motion);
}

int bfm_village_arrival_continue(BfmVillageArrival *a)
{
    return bfm_village_motion_continue(a->motion);
}

BfmForestMessage *bfm_village_arrival_speech(const BfmVillageArrival *a)
{
    BfmVillagePhase phase = bfm_village_arrival_player(a)->phase;
    if (phase != BFM_VILLAGE_FIRST_MESSAGE && phase != BFM_VILLAGE_SECOND_MESSAGE) return NULL;
    return bfm_village_greeting_open(a->scene, a->bytes,
                phase == BFM_VILLAGE_FIRST_MESSAGE ? BFM_VILLAGE_ARRIVAL : BFM_VILLAGE_HUNGER);
}

int bfm_village_arrival_step(BfmVillageArrival *a, BfmGameplayRun *run, int x, int y)
{
    /* The camera's orbit yaw points from the target to the eye. */
    if (!bfm_village_motion_step(a->motion, x, y, (a->camera.yaw + 2048) & 4095)) return 0;
    const BfmVillagePlayer *p = bfm_village_motion_player(a->motion);
    bfm_village_impact_step(a->impact, p->events & BFM_VILLAGE_HIT, run);
    bfm_village_camera_step(&a->camera, p->root.xyz, p->events & BFM_VILLAGE_CAMERA);
    ++a->age;
    return bfm_village_motion_face(a->motion, a->tiles);
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
    while (a->next_card < a->card_count && (int)a->cards[a->next_card].bucket >= depth) {
        const BfmForestDustCard *card = &a->cards[a->next_card++];
        int farther = (int)card->bucket + 1;
        bfm_village_gate_draw_to(a->gate, farther);
        bfm_bedroom_figure_draw_to(a->view.hero, farther);
        bfm_forest_raster_face(&a->surface, &card->material, card->vertices);
    }
    bfm_village_gate_draw_to(a->gate, depth);
    bfm_bedroom_figure_draw_to(a->view.hero, depth);
}

int bfm_village_arrival_draw(BfmVillageArrival *a, const uint8_t flags[64],
                             const BfmForestMessageFrame *window, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_village_motion_player(a->motion);
    BfmVillageCamera drawing = a->camera;
    int shake = bfm_village_impact_shake_y(a->impact);
    drawing.eye[1] += shake;
    drawing.target[1] += shake;
    if (!bfm_village_sky_draw(a->scene + a->descriptor, a->bytes - a->descriptor, pixels) ||
        !bfm_village_field_prepare(&a->view, &drawing, &p->root, &p->pose, flags, a->age,
                                   a->tiles, &pixels[0][0])) return 0;
    const BfmBedroomView *camera = bfm_bedroom_field_view(a->view.field);
    if (!bfm_village_gate_prepare(a->gate, camera, a->tiles, &pixels[0][0])) return 0;
    if (a->residents && !bfm_village_residents_add(a->residents, a->view.hero, camera, a->age))
        return 0;
    a->surface = (BfmForestRaster){.tiles=a->tiles, .pixels=pixels, .top=10, .bottom=229};
    a->next_card = 0;
    a->card_count = bfm_village_impact_project(a->impact, camera, a->cards);
    qsort(a->cards, a->card_count, sizeof *a->cards, far_card_first);
    bfm_bedroom_field_draw(a->view.field, a->tiles, &pixels[0][0],
                           draw_actors, a);
    if (window) {
        int point[3];
        if (!bfm_bedroom_project(camera, p->root.xyz, point)) return 0;
        bfm_bedroom_message_art_draw_side(a->message_art, window, point[0], 0, pixels);
    }
    return 1;
}

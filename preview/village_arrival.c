/* SPDX-License-Identifier: MIT */
#include "village_arrival.h"
#include "forest_message_art.h"
#include "scene_tiles.h"
#include "village_field.h"
#include "village_clock.h"
#include "village_lower.h"
#include "village_leno_delivery.h"
#include "village_earth_return.h"
#include "village_lighting.h"
#include "village_cable.h"
#include "village_impact.h"
#include "village_portrait.h"
#include "village_greeting.h"
#include "village_gate.h"
#include "village_upper_landmarks.h"
#include "village_upper_guard.h"
#include "village_rotor.h"
#include "village_residents.h"
#include "village_sky.h"
#include "southern_woods.h"
#include "woods_box_visit.h"
#include "forest_bank.h"
#include "nightmare_transition.h"
#include "forest_maze.h"
#include "forest_maze_wipe.h"
#include "village_talk_camera.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    BfmForestBank *art;
    BfmVillageMotion *motion;
    BfmVillageCamera camera;
    int radius_goal;
    unsigned age;
} BankVisit;

typedef struct {
    BfmBedroomEntry entry;
    uint16_t tiles[512][1024];
    BfmWoodsBoxVisit *box;
    BfmSouthernWoods *woods;
} WoodsArrival;

typedef struct {
    BfmNightmareTransition wipe;
    enum { EXIT_WALK, MOUNT_RECEIVER, HOLD_PAGE, REVEAL_FIELD } phase;
    uint32_t page[240][320];
    int submitted;
    unsigned reveal_updates;
} WoodsCrossing;

struct BfmVillageArrival {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes, descriptor;
    const MusashiDiscMedia *disc;
    const uint8_t *main;
    size_t main_bytes;
    BfmBedroomEntry *entry;
    BfmVillageLower *lower;
    BfmVillageLenoDelivery *delivery;
    BfmVillageEarthReturn *earth_return;
    BfmSouthernWoods *woods;
    BankVisit *bank;
    BfmWoodsBoxVisit *box;
    WoodsArrival *incoming_woods;
    WoodsCrossing *crossing;
    BfmForestMaze *maze;
    enum { LOWER_STREET, UPPER_FROM_STREET, WOODS_TRAIL, UPPER_FROM_WOODS,
           BANK_FROM_VILLAGE, VILLAGE_FROM_BANK, MAZE_FROM_BANK, BANK_FROM_MAZE } transfer_route;
    unsigned transfer_tick, scene_tick;
    uint16_t (*tiles)[1024];
    BfmVillageMotion *motion;
    BfmVillageCamera camera, presented_camera;
    BfmVillageTalkCamera talk_camera;
    BfmSaveRecord *record;
    int start_talk_camera, latch_window, resident_anchor, resident_right;
    BfmVillageField view;
    BfmVillageDaylight daylight;
    BfmVillageResidents *residents;
    unsigned resident_field_tick;
    BfmVillageGate *gate;
    BfmVillageUpperLandmarks *signs;
    BfmVillageUpperGuard *guard;
    BfmForestMessage *sign_reading;
    unsigned sign_delay;
    int reading_sign, talking_to_guard, first_guard_talk;
    BfmVillageRotor rotor;
    BfmVillageCable *cable;
    BfmVillageImpact *impact;
    BfmForestDustCard cards[BFM_VILLAGE_IMPACT_CARDS];
    unsigned next_card, card_count;
    BfmForestRaster surface;
    BfmForestMessageArt *message_art;
    unsigned age;
};

static void clear_upper(BfmVillageArrival *a)
{
    bfm_village_earth_return_free(a->earth_return);
    bfm_village_leno_delivery_free(a->delivery);
    bfm_forest_message_art_free(a->message_art);
    bfm_village_residents_free(a->residents);
    bfm_village_impact_free(a->impact);
    bfm_village_gate_free(a->gate);
    bfm_forest_message_free(a->sign_reading);
    bfm_village_upper_landmarks_free(a->signs);
    bfm_village_upper_guard_free(a->guard);
    bfm_village_rotor_clear(&a->rotor);
    bfm_village_cable_free(a->cable);
    bfm_village_motion_free(a->motion);
    bfm_village_field_clear(&a->view);
}

static void free_bank(BankVisit *bank)
{
    if (bank) {
        bfm_forest_bank_free(bank->art);
        bfm_village_motion_free(bank->motion);
        free(bank);
    }
}

static void free_woods_arrival(WoodsArrival *pending)
{
    if (!pending) return;
    bfm_woods_box_visit_free(pending->box);
    bfm_southern_woods_free(pending->woods);
    free(pending);
}

void bfm_village_arrival_free(BfmVillageArrival *a)
{
    if (!a) return;
    free(a->crossing);
    free_woods_arrival(a->incoming_woods);
    bfm_woods_box_visit_free(a->box);
    bfm_forest_maze_free(a->maze);
    free_bank(a->bank);
    bfm_southern_woods_free(a->woods);
    bfm_village_lower_free(a->lower);
    clear_upper(a);
    free(a);
}

static int mount_upper(BfmVillageArrival *a, const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                       const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
                       const BfmVillageMotion *departed)
{
    BfmBedroomEntry next = *entry;
    const uint8_t *door = departed ? bfm_village_motion_transfer(departed) : entry->state.destination;
    if (!bfm_castle_room_entry(disc, door, &next) || next.scene_member != 2) return 0;
    a->bytes = bfm_read_archive_field_scene(disc, "SC03.CD", next.scene_member, a->scene);
    const uint32_t room = 0x8018c794u;
    a->descriptor = room - 0x80128158u;
    if (a->descriptor > a->bytes || a->bytes - a->descriptor < 0xcc) return 0;
    a->motion = departed ? bfm_village_motion_arrive(departed, a->scene, a->bytes, room) :
                           bfm_village_motion_open(a->scene, a->bytes, main, main_bytes, room, 5);
    if (!a->motion || !bfm_village_field_load(&a->view, disc, a->scene, a->bytes, room) ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", next.scene_member, tiles)) return 0;
    if (!bfm_village_daylight_begin(&a->daylight, a->scene, a->bytes, room,
                                    next.state.progress[0x37], tiles)) return 0;
    bfm_village_macho_portrait_load(tiles);
    a->message_art = bfm_castle_message_art_load(disc, a->scene, a->bytes, tiles, 0x5da74);
    a->gate = bfm_village_gate_load(a->scene, a->bytes, room, next.state.progress[8], next.state.flags);
    a->signs = bfm_village_upper_landmarks_open(a->scene,a->bytes,room);
    if (!a->signs) return 0;
    if (bfm_village_upper_guard_present(&next.state)) {
        a->guard = bfm_village_upper_guard_open(a->scene,a->bytes,&next.state);
        if (!a->guard || !bfm_village_upper_guard_load_art(a->guard,disc,tiles)) return 0;
    }
    a->impact = bfm_village_impact_open(a->scene, a->bytes, main, main_bytes);
    a->cable = bfm_village_cable_load(a->scene, a->bytes, room);
    if (!a->message_art || !a->gate || !a->impact || !a->cable) return 0;
    if (!bfm_village_rotor_load(&a->rotor, a->scene, a->bytes, u32le(next.state.progress + 8))) return 0;
    if (!(next.state.flags[240 / 8] & 1u) && next.state.progress[8] == 10) {
        a->residents = bfm_village_residents_load(a->scene, a->bytes, room, 0x80188094u, &next.state);
        if (!a->residents) return 0;
    }
    if (bfm_village_earth_return_pending(&next.state)) {
        /* The actor keeps the live record through the conversation. */
        a->earth_return = bfm_village_earth_return_open(disc, a->scene, a->bytes,
                                                       main, main_bytes, &entry->state);
        if (!a->earth_return || !bfm_village_earth_return_load_art(a->earth_return, disc, tiles))
            return 0;
    }
    a->tiles = tiles;
    const int16_t *spawn = bfm_village_motion_player(a->motion)->root.xyz;
    if (departed) {
        bfm_village_camera_lower(&a->camera, spawn, 1);
        if (a->transfer_route == UPPER_FROM_WOODS) {
            a->camera.yaw = 1536;
            bfm_village_camera_lower(&a->camera, spawn, 0);
        }
    } else {
        bfm_village_camera_begin(&a->camera, spawn);
        /* The first live camera update precedes Musashi's first moving update. */
        bfm_village_camera_step(&a->camera, spawn, 0);
    }
    a->presented_camera = a->camera;
    *entry = next;
    a->disc = disc;
    a->main = main;
    a->main_bytes = main_bytes;
    a->entry = entry;
    a->record = &entry->state;
    return 1;
}

BfmVillageArrival *bfm_village_arrival_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                      const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    BfmVillageArrival *a = calloc(1, sizeof *a);
    if (a && !mount_upper(a, disc, entry, main, main_bytes, tiles, NULL)) {
        bfm_village_arrival_free(a);
        return NULL;
    }
    return a;
}

BfmVillageArrival *bfm_village_resume_inn(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                         const uint8_t *main, size_t bytes,
                                         uint16_t tiles[512][1024], BfmForestHud **hud)
{
    BfmVillageArrival *a = calloc(1, sizeof *a);
    if (!a) return NULL;
    a->lower = bfm_village_lower_resume(disc, entry, main, bytes, tiles, hud);
    if (!a->lower) {
        free(a);
        return NULL;
    }
    a->disc = disc;
    a->main = main;
    a->main_bytes = bytes;
    a->entry = entry;
    a->record = &entry->state;
    a->tiles = tiles;
    return a;
}

struct BfmSouthernWoods *bfm_village_arrival_woods(BfmVillageArrival *a)
{
    return a ? a->woods : NULL;
}

BfmRetryScreen *bfm_village_arrival_retry(const BfmVillageArrival *a)
{
    if (!a) return NULL;
    if (a->box) return bfm_woods_box_visit_retry(a->box);
    return a->bank ? bfm_forest_bank_retry(a->bank->art) : bfm_southern_woods_retry(a->woods);
}

int bfm_village_arrival_begin_retry(BfmVillageArrival *a, BfmGameplayRun *run)
{
    if (a->box) return bfm_woods_box_visit_begin_retry(a->box, run);
    if (a->bank) {
        BankVisit *bank = a->bank;
        return bfm_forest_bank_begin_retry(bank->art, a->disc, bank->scene, bank->bytes,
                                           bank->motion, a->record, &bank->camera, &bank->age, run);
    }
    return bfm_southern_woods_begin_retry(a->woods, run);
}

int bfm_village_arrival_draw_retry(BfmVillageArrival *a, const BfmForestHud *hud,
                                 uint32_t pixels[240][320])
{
    if (a->box) return bfm_woods_box_visit_draw_retry(a->box, hud, pixels);
    if (a->bank) return bfm_forest_bank_draw_retry(a->bank->art, a->bank->motion,
                                                  &a->bank->camera, hud, pixels);
    return bfm_southern_woods_draw_retry(a->woods, hud, pixels);
}

int bfm_village_arrival_prepare_recovery(const BfmVillageArrival *a, const BfmGameplayRun *run,
                                        BfmBedroomEntry *out)
{
    if (!a) return 0;
    if (a->box) return bfm_woods_box_visit_prepare_recovery(a->box, run, out);
    if (a->bank) {
        const size_t destination = UINT32_C(0x8018eb3c) - UINT32_C(0x80128158);
        BfmWoodsRetryPage page = bfm_retry_screen_woods_destination(bfm_forest_bank_retry(a->bank->art));
        return bfm_woods_retry_field_destination(a->disc, run, page, a->bank->scene,
                                                 a->bank->bytes, destination, out);
    }
    return bfm_southern_woods_prepare_recovery(a->woods, run, out);
}

uint16_t bfm_village_arrival_room(const BfmVillageArrival *a)
{
    /* The transfer descriptor can change while the outgoing field still draws. */
    if (a->box) return 0x3026;
    if (a->maze) return 0x301b;
    if (a->bank) return 0x3014;
    if (a->woods) return 0x3025;
    if (a->lower) return 0x1010;
    return a->motion ? 0x1011 : 0;
}

const BfmVillagePlayer *bfm_village_arrival_player(const BfmVillageArrival *a)
{
    if (a->box) return bfm_woods_box_visit_player(a->box);
    if (a->maze) return bfm_village_motion_player(bfm_forest_maze_motion(a->maze));
    if (a->bank) return bfm_village_motion_player(a->bank->motion);
    if (a->woods) return bfm_southern_woods_player(a->woods);
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
    if (a->box) return bfm_woods_box_visit_close(a->box, answer);
    if (a->maze) {
        bfm_forest_maze_close_sign(a->maze);
        return 1;
    }
    if (a->bank) return bfm_forest_bank_close(a->bank->art);
    if (a->woods) return bfm_southern_woods_close(a->woods);
    if (a->reading_sign) {
        a->reading_sign = 0;
        return bfm_village_motion_end_read(a->motion);
    }
    if (a->talking_to_guard) {
        bfm_village_upper_guard_close(a->guard);
        if (!a->first_guard_talk) {
            a->talking_to_guard = 0;
            return bfm_village_motion_end_talk(a->motion);
        }
        return 1;
    }
    if (bfm_village_earth_return_message_closed(a->earth_return)) return 1;
    if (bfm_village_leno_delivery_message_closed(a->delivery)) return 1;
    if (a->lower) {
        bfm_village_lower_close(a->lower, answer);
        return 1;
    }
    return bfm_village_arrival_continue(a);
}

void bfm_village_arrival_begin_hotelo_reveal(BfmVillageArrival *a)
{
    bfm_village_lower_begin_hotelo_reveal(a->lower);
}

void bfm_village_arrival_set_scene_tick(BfmVillageArrival *a, unsigned tick)
{
    a->scene_tick = tick;
}

const uint8_t *bfm_village_arrival_castle_transfer(const BfmVillageArrival *a)
{
    return a && !a->transfer_tick ? bfm_village_lower_castle_transfer(a->lower) : NULL;
}

const uint8_t *bfm_village_arrival_shop_transfer(const BfmVillageArrival *arrival)
{
    return bfm_village_lower_shop_transfer(arrival->lower);
}

const uint8_t *bfm_village_arrival_inn_transfer(const BfmVillageArrival *arrival)
{
    return arrival->lower ? bfm_village_lower_inn_transfer(arrival->lower) : NULL;
}

int bfm_village_arrival_maze_crossing(const BfmVillageArrival *a)
{
    return a && a->transfer_tick &&
           (a->transfer_route == MAZE_FROM_BANK || a->transfer_route == BANK_FROM_MAZE);
}

int bfm_village_arrival_forest_score_fades(const BfmVillageArrival *a)
{
    return a && a->transfer_tick == 14 &&
           (a->transfer_route == BANK_FROM_VILLAGE || a->transfer_route == VILLAGE_FROM_BANK ||
            a->transfer_route == MAZE_FROM_BANK || a->transfer_route == BANK_FROM_MAZE);
}

unsigned bfm_village_arrival_reveal_tick(const BfmVillageArrival *a)
{
    /* The common outdoor loader retains a black source frame during the
     * archive read. The maze uses its own outgoing and arriving fade curves. */
    if (bfm_village_arrival_maze_crossing(a)) return BFM_MAZE_REVEAL_TICK;
    if (a->transfer_route == WOODS_TRAIL || a->transfer_route == UPPER_FROM_WOODS ||
        a->transfer_route == BANK_FROM_VILLAGE || a->transfer_route == VILLAGE_FROM_BANK) return 195;
    return a->transfer_route == UPPER_FROM_STREET ? 138 : 145;
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

/* A visit can begin on the upper gondola or at a saved inn. Reopen the
 * incoming upper field from the same mounted resources in both cases. */
static int climb_to_upper(BfmVillageArrival *a)
{
    BfmVillageLower *lower = a->lower;
    BfmSouthernWoods *woods = a->woods;
    const BfmVillageMotion *departed = woods ? bfm_southern_woods_departure(woods) :
                                               bfm_village_lower_departure(lower);
    const MusashiDiscMedia *disc = a->disc;
    BfmBedroomEntry *entry = a->entry;
    const uint8_t *main = a->main;
    size_t main_bytes = a->main_bytes;
    uint16_t (*tiles)[1024] = a->tiles;
    unsigned transfer_tick = a->transfer_tick, scene_tick = a->scene_tick;
    int carrying = woods && bfm_southern_woods_carrying_leno(woods);
    clear_upper(a);
    memset(a, 0, sizeof *a);
    a->lower = lower;
    a->woods = woods;
    a->transfer_route = woods ? UPPER_FROM_WOODS : UPPER_FROM_STREET;
    if (!departed || !mount_upper(a, disc, entry, main, main_bytes, tiles, departed)) return 0;
    if (bfm_village_leno_delivery_pending(&entry->state, carrying)) {
        const BfmVillagePlayer *player = bfm_village_motion_player(a->motion);
        a->delivery = bfm_village_leno_delivery_open(a->scene, a->bytes, main, main_bytes,
                                                     &entry->state, carrying, &player->root);
        if (!a->delivery) return 0;
    }
    a->lower = NULL;
    a->woods = NULL;
    bfm_village_lower_free(lower);
    bfm_southern_woods_free(woods);
    a->transfer_tick = transfer_tick;
    a->scene_tick = scene_tick;
    return 1;
}

/* The maze's route and guide belong to the same save as the current field.
 * Publish only these two changes into the shared player after the mount. */
static void sync_maze_state(const BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (!run) return;
    unsigned mask = 1u << (322%8);
    run->story_flags[322/8] = (uint8_t)((run->story_flags[322/8] & ~mask) |
                                       (save->flags[322/8] & mask));
    run->banks.variables[204] = save->variables[204];
}

static int enter_maze(BfmVillageArrival *a, BfmGameplayRun *run)
{
    const BfmVillageMotion *departed = a->bank->motion;
    BfmBedroomEntry next = *a->entry;
    if (!bfm_castle_room_entry(a->disc,bfm_village_motion_transfer(departed),&next) ||
        next.scene_member != 7) return 0;
    bfm_forest_maze_gate_depart(bfm_village_motion_maze_gate(departed),
                               next.state.flags,next.state.variables);
    BfmForestMaze *maze = bfm_forest_maze_open(a->disc,a->main,a->main_bytes,&next.state);
    if (!maze) return 0;
    free_bank(a->bank);
    a->bank = NULL;
    a->maze = maze;
    *a->entry = next;
    sync_maze_state(a->record,run);
    return 1;
}

static int enter_bank(BfmVillageArrival *a, BfmGameplayRun *run)
{
    const BfmVillageMotion *departed = a->maze ? bfm_forest_maze_motion(a->maze) :
                                                 bfm_village_lower_departure(a->lower);
    BfmBedroomEntry next = *a->entry;
    if (!departed || !bfm_castle_room_entry(a->disc, bfm_village_motion_transfer(departed), &next) ||
        next.scene_member != 6) return 0;
    if (a->maze) bfm_forest_maze_gate_depart(bfm_village_motion_maze_gate(departed),
                                            next.state.flags,next.state.variables);
    BankVisit *bank = calloc(1, sizeof *bank);
    if (!bank) return 0;
    size_t bytes = bfm_read_archive_field_scene(a->disc, "SC03.CD", 6, bank->scene);
    bank->bytes = bytes;
    bank->motion = bfm_village_motion_bank(bank->scene, bytes, a->main, a->main_bytes,
                                          next.state.destination[2], u32le(next.state.progress + 8));
    bank->art = bfm_forest_bank_open(a->disc, bank->scene, bytes, a->tiles, next.state.progress[0x37]);
    if (!bank->motion || !bank->art) {
        free_bank(bank);
        return 0;
    }
    const BfmVillagePlayer *p = bfm_village_motion_player(bank->motion);
    bfm_village_camera_bank(&bank->camera, &bank->radius_goal, p->root.xyz, 1, 0);
    if (a->maze) {
        bfm_forest_maze_free(a->maze);
        a->maze = NULL;
        sync_maze_state(&next.state,run);
    } else {
        bfm_village_lower_free(a->lower);
        a->lower = NULL;
    }
    bfm_forest_bank_bind_player(bank->art,a->main,a->main_bytes);
    a->bank = bank;
    *a->entry = next;
    return 1;
}

static int return_from_bank(BfmVillageArrival *a, BfmGameplayRun *run)
{
    BfmVillageLower *lower = bfm_village_lower_open(a->disc, a->entry, a->bank->motion, a->tiles);
    if (!lower) return 0;
    free_bank(a->bank);
    a->bank = NULL;
    a->lower = lower;
    bfm_village_lower_register_actors(lower, run);
    return 1;
}

/* Both renderers borrow the canonical entry and tile addresses. Mount and draw
 * the receiver there, retaining its contents until the outgoing scene is done. */
static int prepare_woods_arrival(BfmVillageArrival *a, const uint8_t *door,
                                const BfmBedroomEntry *restored)
{
    uint32_t target = u32le(door);
    if (a->incoming_woods || (target != 0x00013026 && target != 0x00033025 &&
        !(restored && target == 0x00ff3026))) return 0;
    WoodsArrival *pending = calloc(1, sizeof *pending);
    if (!pending) return 0;
    uint32_t (*frame)[320] = malloc(240 * sizeof *frame);
    if (!frame) { free(pending); return 0; }
    BfmBedroomEntry previous = *a->entry;
    memcpy(pending->tiles, a->tiles, sizeof pending->tiles);
    int drawn = 0;
    if (target == 0x00033025) {
        pending->woods = bfm_southern_woods_open(a->disc, a->entry, door, a->main,
                                                  a->main_bytes, a->tiles);
        if (pending->woods)
            drawn = bfm_southern_woods_draw(pending->woods, a->record->flags, NULL, frame);
    } else {
        if (restored) *a->entry = *restored;
        if (restored || bfm_castle_room_entry(a->disc, door, a->entry)) {
            pending->box = bfm_woods_box_visit_open(a->disc, a->entry, a->main,
                                                    a->main_bytes, a->tiles);
            if (pending->box)
                drawn = bfm_woods_box_visit_draw(pending->box, a->record->flags, NULL, frame);
        }
    }
    free(frame);
    pending->entry = *a->entry;
    *a->entry = previous;
    if (!drawn) {
        memcpy(a->tiles, pending->tiles, sizeof pending->tiles);
        free_woods_arrival(pending);
        return 0;
    }
    for (unsigned y = 0; y < 512; ++y) {
        uint16_t incoming[1024];
        memcpy(incoming, a->tiles[y], sizeof incoming);
        memcpy(a->tiles[y], pending->tiles[y], sizeof incoming);
        memcpy(pending->tiles[y], incoming, sizeof incoming);
    }
    a->incoming_woods = pending;
    return 1;
}

static void install_woods_arrival(BfmVillageArrival *a)
{
    WoodsArrival *pending = a->incoming_woods;
    free_bank(a->bank);
    a->bank = NULL;
    bfm_woods_box_visit_free(a->box);
    bfm_southern_woods_free(a->woods);
    *a->entry = pending->entry;
    memcpy(a->tiles, pending->tiles, sizeof pending->tiles);
    a->woods = pending->woods;
    a->box = pending->box;
    free(pending);
    a->incoming_woods = NULL;
}

static WoodsCrossing *open_woods_crossing(const MusashiDiscMedia *disc)
{
    WoodsCrossing *c = calloc(1, sizeof *c);
    if (c && !bfm_nightmare_transition_open(disc, &c->wipe)) {
        free(c);
        return NULL;
    }
    return c;
}

int bfm_village_arrival_restore_woods_box(BfmVillageArrival *a, const BfmBedroomEntry *restored)
{
    if (!a || !restored || a->crossing || (!a->woods && !a->bank && !a->box) || restored->scene_member != 11 ||
        u32le(restored->state.destination) != 0x00ff3026) return 0;
    WoodsCrossing *c = open_woods_crossing(a->disc);
    if (!c) return 0;
    if (!prepare_woods_arrival(a, restored->state.destination, restored)) { free(c); return 0; }
    install_woods_arrival(a);
    bfm_nightmare_transition_begin_arrival(&c->wipe);
    c->phase = REVEAL_FIELD;
    a->crossing = c;
    a->transfer_tick = 0;
    return 1;
}

/* The two Cleft gates share the resident exit delay, the loader's captured
 * eight-line curtain and the incoming field fade. The receiving actor is live
 * from its first shaded frame. Neither gate issues the Nightmare audio stop. */
static int begin_woods_crossing(BfmVillageArrival *a)
{
    WoodsCrossing *c = open_woods_crossing(a->disc);
    if (!c) return 0;
    bfm_nightmare_transition_begin_departure(&c->wipe);
    a->crossing = c;
    return 1;
}

static int step_box_field(BfmWoodsBoxVisit *box, BfmGameplayRun *run, BfmFieldInput field,
                          unsigned actor_tick, BfmForestMessage **opened)
{
    if (opened) return bfm_woods_box_visit_step_at(box, run, field, actor_tick, opened);
    /* Direction-only callers have no dialogue owner to accept a new prompt. */
    BfmForestMessage *unused = NULL;
    field.square_pressed = 0;
    int result = bfm_woods_box_visit_step_at(box, run, field, actor_tick, &unused);
    bfm_forest_message_free(unused);
    return result;
}

static int step_woods_field(BfmVillageArrival *a, BfmGameplayRun *run, BfmFieldInput field,
                            BfmForestMessage **opened)
{
    unsigned tick = a->scene_tick++;
    return a->box ? step_box_field(a->box, run, field, tick, opened) :
                    bfm_southern_woods_step_field_at(a->woods, run, field, tick, opened);
}

static int step_woods_crossing(BfmVillageArrival *a, BfmGameplayRun *run, BfmFieldInput field,
                               BfmForestMessage **opened)
{
    WoodsCrossing *c = a->crossing;
    if (c->phase == EXIT_WALK && !step_woods_field(a, run, (BfmFieldInput){0}, opened)) return 0;
    BfmNightmareTransitionEvent event = BFM_NIGHTMARE_TRANSITION_NONE;
    if (c->phase != MOUNT_RECEIVER) {
        event = bfm_nightmare_transition_step(&c->wipe, 1, c->submitted, a->incoming_woods != NULL);
        if (event == BFM_NIGHTMARE_CAPTURE_OUTGOING) c->phase = MOUNT_RECEIVER;
    }
    if (c->phase == MOUNT_RECEIVER) {
        const BfmVillageMotion *motion = a->box ? bfm_woods_box_visit_departure(a->box) :
                                                 bfm_southern_woods_departure(a->woods);
        const uint8_t *door = motion ? bfm_village_motion_transfer(motion) : NULL;
        if (!door || !prepare_woods_arrival(a, door, NULL)) return 0;
        c->phase = HOLD_PAGE;
    } else if (event == BFM_NIGHTMARE_ENTER_VILLAGE) {
        /* Elapsed playtime is maintained by the host field loop. Preserve
         * its current value across the asynchronous transfer. An earned FF
         * restart instead installs the checkpoint's own timer. */
        memcpy(a->incoming_woods->entry.state.progress + 4, a->entry->state.progress + 4, 4);
        install_woods_arrival(a);
        c->submitted = 0;
        c->phase = REVEAL_FIELD;
        bfm_nightmare_transition_begin_arrival(&c->wipe);
    }
    if (c->phase != REVEAL_FIELD) return 1;
    /* Physical input starts in the live Cleft field, after the final reveal
     * update. The entrance script and the world continue during the wipe. */
    if (a->woods) field = (BfmFieldInput){0};
    if (!step_woods_field(a, run, field, opened)) return 0;
    if (event == BFM_NIGHTMARE_REVEAL_COMPLETE) {
        free(c);
        a->crossing = NULL;
    } else if (event != BFM_NIGHTMARE_ENTER_VILLAGE) ++c->reveal_updates;
    return 1;
}

int bfm_village_arrival_woods_playtime_paused(const BfmVillageArrival *a)
{
    return a && a->crossing;
}

void bfm_village_arrival_submit_woods_crossing(BfmVillageArrival *a, const uint32_t submitted[240][320],
                                              const uint32_t presented[240][320])
{
    WoodsCrossing *c = a ? a->crossing : NULL;
    if (!c) return;
    if (!c->submitted && (c->phase == HOLD_PAGE || c->phase == REVEAL_FIELD)) {
        /* On departure the loader captures the page already on the display.
         * The last walking frame was submitted to the other buffer. On entry
         * both old queued pages must yield to the actual receiving field. */
        memcpy(c->page, c->phase == HOLD_PAGE ? presented : submitted, sizeof c->page);
        c->submitted = 1;
    }
}

int bfm_village_arrival_draw_woods_crossing(const BfmVillageArrival *a, uint32_t pixels[240][320])
{
    const WoodsCrossing *c = a ? a->crossing : NULL;
    if (!c) return 0;
    if (c->submitted && (c->phase == HOLD_PAGE || (c->phase == REVEAL_FIELD && c->reveal_updates < 2)))
        memcpy(pixels, c->page, sizeof c->page);
    bfm_nightmare_transition_draw(&c->wipe, pixels);
    return 1;
}

void bfm_village_arrival_lower_register(BfmVillageArrival *a, BfmGameplayRun *run)
{
    if (a->lower) bfm_village_lower_register_actors(a->lower, run);
}

void bfm_village_arrival_lower_ambient(BfmVillageArrival *a, BfmGameplayRun *run)
{
    if (a->lower) bfm_village_lower_step_ambient(a->lower, run);
}

static int step_earth_return(BfmVillageArrival *a, BfmGameplayRun *run, BfmForestMessage **opened)
{
    int ready = bfm_village_motion_player(a->motion)->controlled;
    int result = bfm_village_earth_return_step(a->earth_return, a->motion, &a->camera,
                                              run, ready, opened);
    if (result == 2) {
        a->presented_camera = a->camera;
        ++a->age;
        if (!bfm_village_motion_face(a->motion, a->tiles)) return 0;
    }
    return result;
}

/* The actor-relative view consumes the same packed station format as the
 * hillside resident. Lumpwood can speak from his new position after stepping
 * aside, so the current station comes from his live actor. */
static int begin_guard_talk(BfmVillageArrival *a, const BfmVillageGuardTalk *talk)
{
    if (talk->first) {
        uint8_t station[8];
        for (unsigned axis = 0; axis < 4; ++axis) {
            unsigned value = axis < 3 ? (uint16_t)talk->speaker[axis] : talk->speaker_yaw;
            station[2*axis] = (uint8_t)value;
            station[2*axis+1] = (uint8_t)(value >> 8);
        }
        if (!bfm_village_motion_begin_talk_at(a->motion,talk->speaker,talk->approach) ||
            !bfm_village_talk_camera_frame(&a->talk_camera,station,talk->camera,
                                           &a->presented_camera)) return 0;
    } else if (!bfm_village_motion_begin_talk_hold(a->motion)) return 0;
    a->talking_to_guard = 1;
    a->first_guard_talk = talk->first;
    return 1;
}

static int upper_contact(BfmVillageArrival *a, const int16_t previous[3])
{
    const BfmVillagePlayer *p = bfm_village_motion_player(a->motion);
    if (!p->controlled || p->phase != BFM_VILLAGE_CONTROL) return 1;
    int16_t foot[3];
    memcpy(foot,p->root.xyz,sizeof foot);
    bfm_village_upper_landmarks_slide(a->signs,previous,foot);
    bfm_village_upper_guard_contact(a->guard,previous,foot);
    return !memcmp(foot,p->root.xyz,sizeof foot) || bfm_village_motion_correct_contact(a->motion,foot);
}

static int step_field(BfmVillageArrival *a, BfmGameplayRun *run,
                      BfmFieldInput field, BfmForestMessage **opened)
{
    int x = field.x, y = field.y, talk_edge = field.square_pressed;
    if (opened) *opened = NULL;
    if (a->crossing) return step_woods_crossing(a, run, field, opened);
    /* The upper field and the lower street share the running world clock. The
     * player is already on the incoming actor list for its 17 fade-in updates. */
    unsigned reveal = bfm_village_arrival_reveal_tick(a);
    /* Actors read the shared counter before the field advances it. */
    unsigned actor_tick = a->scene_tick;
    unsigned residents_before = a->resident_field_tick;
    if (!a->transfer_tick || a->transfer_tick >= reveal - 2) ++a->scene_tick;
    if (a->residents) ++a->resident_field_tick;
    if (a->transfer_tick) {
        /* The outgoing field freezes at its exit contact. The incoming mesh is
         * ready before the crossfade makes its first scanout visible. */
        ++a->transfer_tick;
        /* The cleft changes the logical room while the last village frame
         * is still on screen. Its woods field appears later in the wipe. */
        if (a->transfer_tick == 30 &&
            (a->transfer_route == WOODS_TRAIL || a->transfer_route == UPPER_FROM_WOODS)) {
            const BfmVillageMotion *departed = a->woods ? bfm_southern_woods_departure(a->woods) :
                                                         a->motion;
            if (!bfm_castle_room_entry(a->disc,bfm_village_motion_transfer(departed),a->entry)) return 0;
        }
        if (bfm_village_arrival_maze_crossing(a) &&
            a->transfer_tick >= BFM_MAZE_SOURCE_DISPATCH_TICK &&
            a->transfer_tick <= BFM_MAZE_LAST_SOURCE_TICK) {
            if (a->maze) {
                if (!bfm_forest_maze_departing(a->maze,run,a->record)) return 0;
            } else {
                BankVisit *bank = a->bank;
                if (!bfm_village_motion_maze_departing(bank->motion)) return 0;
                const BfmBedroomRoot *root = &bfm_village_motion_player(bank->motion)->root;
                bfm_village_camera_bank(&bank->camera,&bank->radius_goal,root->xyz,0,0);
                bfm_forest_bank_step_actors(bank->art,root,run,a->record);
                ++bank->age;
            }
        }
        if (a->transfer_tick == reveal) {
            if (a->transfer_route == WOODS_TRAIL) {
                a->woods = bfm_southern_woods_open(a->disc, a->entry,
                    bfm_village_motion_transfer(a->motion), a->main, a->main_bytes, a->tiles);
                if (!a->woods) return 0;
            } else if (a->transfer_route == MAZE_FROM_BANK) {
                if (!enter_maze(a,run)) {
                    --a->transfer_tick;
                    a->scene_tick = actor_tick;
                    a->resident_field_tick = residents_before;
                    return 0;
                }
            } else if (a->transfer_route == BANK_FROM_VILLAGE || a->transfer_route == BANK_FROM_MAZE) {
                if (!enter_bank(a,run)) {
                    --a->transfer_tick;
                    a->scene_tick = actor_tick;
                    a->resident_field_tick = residents_before;
                    return 0;
                }
            } else if (a->transfer_route == VILLAGE_FROM_BANK) {
                if (!return_from_bank(a, run)) return 0;
            } else if (a->transfer_route != LOWER_STREET) {
                if (!climb_to_upper(a)) return 0;
                if (run && a->delivery) {
                    memcpy(run->player.record, a->record->progress, sizeof run->player.record);
                    memcpy(run->banks.variables, a->record->variables, sizeof run->banks.variables);
                }
            } else {
                a->lower = bfm_village_lower_open(a->disc, a->entry, a->motion, a->tiles);
                if (!a->lower) return 0;
                bfm_village_lower_register_actors(a->lower, run);
            }
        }
        if (a->transfer_tick < reveal + (a->transfer_route == WOODS_TRAIL ? 2 : 16)) {
            if (a->transfer_tick >= reveal &&
                (a->transfer_route == LOWER_STREET || a->transfer_route == VILLAGE_FROM_BANK))
                bfm_village_lower_step_ambient(a->lower, run);
            if (a->transfer_route == WOODS_TRAIL && a->transfer_tick == reveal + 1)
                bfm_southern_woods_rescue_field_step(a->woods, run, actor_tick);
            if (a->transfer_tick < reveal) return 1;
            if (a->earth_return &&
                (a->transfer_route == UPPER_FROM_WOODS || a->transfer_route == UPPER_FROM_STREET))
                return step_earth_return(a, run, opened) != 0;
            if (!a->bank && !a->maze && a->transfer_route != VILLAGE_FROM_BANK && a->delivery) {
                /* Newly registered actors update before the screen has opened
                 * enough for the player to take control. */
                int result = bfm_village_leno_delivery_step(a->delivery, a->motion, &a->camera,
                                                            0, opened);
                return result != 0;
            }
            /* The bank arrival walk runs while the incoming field brightens. */
            if (!a->bank && !a->maze) return 1;
        }
        if (a->transfer_tick >= reveal + 21) a->transfer_tick = 0;
    }
    if (a->maze) {
        if (!bfm_forest_maze_step(a->maze,field,run,a->record,opened)) return 0;
        if (!a->transfer_tick && bfm_village_motion_transfer(bfm_forest_maze_motion(a->maze))) {
            a->transfer_route = BANK_FROM_MAZE;
            a->transfer_tick = 1;
        }
        return 1;
    }
    if (a->bank) {
        BankVisit *bank = a->bank;
        const BfmVillagePlayer *p = bfm_village_motion_player(bank->motion);
        int16_t previous[3];
        memcpy(previous,p->root.xyz,sizeof previous);
        if (run) memcpy(run->player.record,a->record->progress,sizeof run->player.record);
        int recovery = bfm_forest_bank_recovery_step(bank->art, bank->motion, run, a->record, opened);
        if (recovery == BFM_BANK_REVIVED) {
            field = (BfmFieldInput){0};
            x = y = 0;
        } else if (recovery != 0) return recovery > 0;
        if (!bfm_forest_bank_awarding(bank->art)) {
            bfm_village_clock_step(a->record,0);
            if (run) memcpy(run->player.record,a->record->progress,sizeof run->player.record);
        }
        int hit = bfm_forest_bank_receive(bank->art, bank->motion, run, field, actor_tick);
        if (!hit) return 0;
        if (run) memcpy(a->record->progress,run->player.record,sizeof a->record->progress);
        if (hit == BFM_BANK_HIT_FATAL) return 1;
        if (!bfm_forest_bank_move(bank->art,bank->motion,run,field,(bank->camera.yaw+2048)&4095))
            return 0;
        if (bfm_village_motion_transfer(bank->motion)) {
            if (!a->transfer_tick) {
                a->transfer_route = p->destination == 0x301b ? MAZE_FROM_BANK : VILLAGE_FROM_BANK;
                a->transfer_tick = 1;
            }
        } else {
            p = bfm_village_motion_player(bank->motion);
            int16_t tracked[3] = {p->root.xyz[0],bfm_forest_bank_tracking_y(bank->art,p->root.xyz[1]),
                                  p->root.xyz[2]};
            bfm_village_camera_bank(&bank->camera, &bank->radius_goal, tracked, 0, field.camera_pressed);
            bfm_forest_bank_step_actors(bank->art,&p->root,run,a->record);
            if (!bfm_forest_bank_cast(bank->art,bank->motion,run,&bank->camera,a->record,field,
                                     bank->age,actor_tick,previous,opened)) return 0;
            bfm_forest_bank_step_light(bank->art, a->record->progress[0x37]);
            ++bank->age;
        }
        return 1;
    }
    if (a->box) {
        if (!step_box_field(a->box, run, field, actor_tick, opened)) return 0;
        const BfmVillageMotion *motion = bfm_woods_box_visit_departure(a->box);
        if (!a->transfer_tick && motion) {
            const uint8_t *door = bfm_village_motion_transfer(motion);
            return door && u32le(door) == 0x00033025 && begin_woods_crossing(a);
        }
        return 1;
    }
    if (a->woods) {
        if (!bfm_southern_woods_step_field_at(a->woods, run, field, actor_tick, opened)) return 0;
        const BfmVillageMotion *motion = bfm_southern_woods_departure(a->woods);
        if (!a->transfer_tick && motion) {
            const uint8_t *door = bfm_village_motion_transfer(motion);
            if (!door) return 0;
            if (u32le(door) == 0x00013026) return begin_woods_crossing(a);
            a->transfer_route = UPPER_FROM_WOODS;
            a->transfer_tick = 1;
        }
        return 1;
    }
    if (a->lower) {
        if (!bfm_village_lower_step_gameplay(a->lower, run, field, opened)) return 0;
        if (!a->transfer_tick && bfm_village_lower_departure(a->lower)) {
            const BfmVillagePlayer *p = bfm_village_lower_player(a->lower);
            a->transfer_route = p->destination == 0x3014 ? BANK_FROM_VILLAGE : UPPER_FROM_STREET;
            a->transfer_tick = 1;
        }
        return 1;
    }
    if (a->earth_return) {
        int step = step_earth_return(a, run, opened);
        if (step != 1) return step != 0;
    }
    if (a->delivery) {
        int step = bfm_village_leno_delivery_step(a->delivery, a->motion, &a->camera, 1, opened);
        if (!step) return 0;
        if (step == 2) {
            const BfmVillagePlayer *hero = bfm_village_motion_player(a->motion);
            bfm_village_camera_lower(&a->camera, hero->root.xyz, 0);
            a->presented_camera = a->camera;
            ++a->age;
            return bfm_village_motion_face(a->motion, a->tiles);
        }
    }
    const BfmVillagePlayer *before = bfm_village_motion_player(a->motion);
    BfmVillageGuardTalk guard_talk;
    if (opened && before->controlled && !a->transfer_tick &&
        bfm_village_upper_guard_begin(a->guard,a->record,before->root.xyz,talk_edge,&guard_talk)) {
        if (!begin_guard_talk(a,&guard_talk)) return 0;
        if (run) run->banks.variables[64] = a->record->variables[64];
    } else if (opened && before->controlled && talk_edge &&
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
    if (opened && before->controlled && !a->transfer_tick && talk_edge) {
        int16_t sign[3];
        a->sign_reading = bfm_village_upper_landmarks_read(a->signs,before->root.xyz,talk_edge,sign);
        if (a->sign_reading) {
            if (!bfm_village_motion_begin_read(a->motion,sign)) return 0;
            a->sign_delay = 4;
            a->reading_sign = 1;
        }
    }
    /* The camera's orbit yaw points from the target to the eye. */
    int movement_yaw = (a->camera.yaw + 2048) & 4095;
    if (a->talk_camera.phase)
        movement_yaw = bfm_village_motion_view_yaw(a->presented_camera.eye, a->presented_camera.target);
    bfm_village_daylight_step(&a->daylight, a->record->progress[0x37], a->tiles);
    int16_t previous[3];
    memcpy(previous, before->root.xyz, sizeof previous);
    if (!bfm_village_motion_step_field(a->motion, x, y, movement_yaw, run) ||
        !bfm_village_motion_resident_contact(a->motion, a->residents, a->resident_field_tick, previous) ||
        !upper_contact(a,previous) || !bfm_village_residents_step(a->residents, a->record)) return 0;
    const BfmVillagePlayer *p = bfm_village_motion_player(a->motion);
    if (p->events & BFM_VILLAGE_TRANSFER) {
        a->transfer_route = p->destination == 0x3025 ? WOODS_TRAIL : LOWER_STREET;
        a->transfer_tick = 1;
    }
    if (p->controlled && !a->transfer_tick && !bfm_village_residents_talking(a->residents))
        bfm_village_clock_step(a->record, 0);
    bfm_village_cable_step(a->cable, p->movement, p->movement <= 81, p->root.xyz, run);
    bfm_village_impact_step(a->impact, p->events & BFM_VILLAGE_HIT, run);
    if (a->transfer_route == UPPER_FROM_WOODS)
        bfm_village_camera_lower(&a->camera, p->root.xyz, 0);
    else
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
    if (a->talking_to_guard && opened)
        *opened = bfm_village_upper_guard_message(a->guard,bfm_village_motion_talk_ready(a->motion));
    if (a->sign_delay && !--a->sign_delay) {
        if (!opened) return 0;
        *opened = a->sign_reading;
        a->sign_reading = NULL;
    }
    a->presented_camera = a->camera;
    bfm_village_talk_camera_step(&a->talk_camera, &a->presented_camera);
    if (a->talking_to_guard && a->first_guard_talk && !a->talk_camera.phase) {
        bfm_village_upper_guard_camera_done(a->guard);
        if (!bfm_village_upper_guard_busy(a->guard)) {
            a->talking_to_guard = 0;
            if (!bfm_village_motion_end_talk(a->motion)) return 0;
        }
    }
    ++a->age;
    return bfm_village_motion_face(a->motion, a->tiles);
}

int bfm_village_arrival_step_field(BfmVillageArrival *a, BfmGameplayRun *run,
                                   BfmFieldInput field, BfmForestMessage **opened)
{
    /* The upper actor starts on the update after admission and stops when
     * its outgoing field freezes. Musashi updates before the rotor when
     * both actors use the shared random stream in the same field. */
    int advance_upper = !a->lower && !a->woods && !a->bank && !a->box && !a->maze && a->motion &&
        (!a->transfer_tick || a->transfer_tick >= bfm_village_arrival_reveal_tick(a));
    int result = step_field(a, run, field, opened);
    if (result && advance_upper) {
        bfm_village_rotor_step(&a->rotor, run, u32le(a->record->progress + 8));
        if (bfm_village_upper_guard_step(a->guard)) bfm_village_talk_camera_release(&a->talk_camera);
    }
    return result;
}

int bfm_village_arrival_award_cue(const BfmVillageArrival *a)
{
    if (a->bank) return bfm_forest_bank_award_cue(a->bank->art);
    return a->woods && bfm_southern_woods_award_cue(a->woods);
}

void bfm_village_arrival_woods_score_applied(BfmVillageArrival *a, unsigned selector)
{
    if (a && a->box) bfm_woods_box_visit_score_applied(a->box, selector);
    if (a && a->earth_return && bfm_village_arrival_room(a) == 0x1011)
        bfm_village_earth_return_score_applied(a->earth_return, selector);
}

unsigned bfm_village_arrival_woods_score_selector(const BfmVillageArrival *a)
{
    /* Both fields initialize the same ordinary score, including a return to
     * the working Memory Box. The room owner retains an already playing cue. */
    if (a && a->box) return bfm_woods_box_visit_score(a->box);
    return a && a->woods ? 0x18u : 0;
}

int bfm_village_arrival_field_score(const BfmVillageArrival *a, unsigned village_score)
{
    if (a && a->earth_return && bfm_village_arrival_room(a) == 0x1011) {
        unsigned track = bfm_village_earth_return_score(a->earth_return, village_score);
        return track ? (int)track : -1;
    }
    unsigned track = bfm_village_arrival_woods_score_selector(a);
    if (!track && a && a->box) return -1;
    return (int)track;
}

unsigned bfm_village_arrival_take_score_command(BfmVillageArrival *a)
{
    return a ? bfm_village_lower_take_score_command(a->lower) : 0;
}

unsigned bfm_village_arrival_take_upper_sound(BfmVillageArrival *a)
{
    return a && a->earth_return && bfm_village_arrival_room(a) == 0x1011
        ? bfm_village_earth_return_take_sound(a->earth_return) : 0;
}

unsigned bfm_village_arrival_woods_sound(const BfmVillageArrival *a)
{
    if (a->bank) return bfm_forest_bank_sound(a->bank->art);
    return a->woods ? bfm_southern_woods_sound(a->woods) : 0;
}

int bfm_village_arrival_take_retired_window(BfmVillageArrival *a)
{
    return a && bfm_woods_box_visit_take_retired_window(a->box);
}

unsigned bfm_village_arrival_take_box_voice(BfmVillageArrival *a)
{
    return a ? bfm_woods_box_visit_take_voice(a->box) : 0;
}

unsigned bfm_village_arrival_take_box_sound(BfmVillageArrival *a)
{
    return a && a->box ? bfm_woods_box_visit_take_sound(a->box) : 0;
}

size_t bfm_village_arrival_take_woods_sounds(BfmVillageArrival *a, unsigned *out, size_t capacity)
{
    if (a && a->maze && out && capacity) {
        size_t count = 0;
        while (count < capacity && (out[count] = bfm_forest_maze_take_sound(a->maze)) != 0) ++count;
        return count;
    }
    if (a && a->bank && out && capacity) {
        size_t count = 0;
        while (count < capacity && (out[count] = bfm_forest_bank_take_sound(a->bank->art)) != 0) ++count;
        return count;
    }
    return a && a->woods ? bfm_southern_woods_take_sounds(a->woods, out, capacity) : 0;
}

int bfm_village_arrival_take_hud(BfmVillageArrival *a)
{
    if (!a) return 0;
    if (a->box) return bfm_woods_box_visit_take_hud(a->box);
    return bfm_village_earth_return_take_hud(a->earth_return);
}

int bfm_village_arrival_rest_hud(const BfmVillageArrival *a, BfmForestHud *hud)
{
    if (a && a->box) return bfm_woods_box_visit_rest_hud(a->box, hud);
    return bfm_village_lower_rest_hud(a ? a->lower : NULL, hud);
}

int bfm_village_arrival_detector(const BfmVillageArrival *a)
{
    return bfm_southern_woods_detector(a->woods);
}

int bfm_village_arrival_tether_gauge(const BfmVillageArrival *a)
{
    if (a->bank) return bfm_forest_bank_tether(a->bank->art);
    return a->woods ? bfm_southern_woods_tether_gauge(a->woods) : -1;
}

int bfm_village_arrival_step_input(BfmVillageArrival *a, BfmGameplayRun *run, int x, int y,
                                   int talk_edge, BfmForestMessage **opened)
{
    return bfm_village_arrival_step_field(a, run,
        (BfmFieldInput){.x=x, .y=y, .square_pressed=talk_edge}, opened);
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
        int signs = bfm_village_upper_landmarks_next_depth(a->signs);
        if (signs > bucket) bucket = signs;
        if (hero > bucket) bucket = hero;
        if (a->next_card < a->card_count && (int)a->cards[a->next_card].bucket > bucket)
            bucket = (int)a->cards[a->next_card].bucket;
        if (bucket == INT_MIN || bucket < depth) break;

        while (a->next_card < a->card_count && (int)a->cards[a->next_card].bucket == bucket) {
            const BfmForestDustCard *card = &a->cards[a->next_card++];
            bfm_forest_raster_face(&a->surface, &card->material, card->vertices);
        }
        bfm_village_gate_draw_to(a->gate, bucket);
        bfm_village_upper_landmarks_draw_to(a->signs,bucket);
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
    if (a->box) return bfm_woods_box_visit_draw(a->box, flags, window, pixels);
    if (a->maze) return bfm_forest_maze_draw(a->maze,flags,window,pixels);
    if (a->bank) {
        if (!bfm_forest_bank_draw(a->bank->art, a->bank->motion, &a->bank->camera,
                                  a->record, a->bank->age, pixels)) return 0;
        if (window) bfm_forest_bank_draw_message(a->bank->art, window, pixels);
        return 1;
    }
    if (a->woods) return bfm_southern_woods_draw(a->woods, flags, window, pixels);
    if (a->lower) return bfm_village_lower_draw(a->lower, a->scene_tick, flags, window, pixels);
    const BfmVillagePlayer *p = bfm_village_motion_player(a->motion);
    BfmBedroomPose displayed = p->pose;
    bfm_village_leno_delivery_pose(a->delivery, &displayed);
    BfmVillageCamera drawing = a->presented_camera;
    int shake = bfm_village_impact_shake_y(a->impact);
    drawing.eye[1] += shake;
    drawing.target[1] += shake;
    if (!bfm_bedroom_figure_lights(a->view.hero, a->daylight.lamps) ||
        !bfm_village_sky_draw_lit(a->scene + a->descriptor, a->bytes - a->descriptor,
                                  a->daylight.sky, pixels) ||
        !bfm_village_field_prepare(&a->view, &drawing, &p->root, &displayed, flags, a->age,
                                   a->tiles, &pixels[0][0])) return 0;
    const BfmBedroomView *camera = bfm_bedroom_field_view(a->view.field);
    if (!bfm_village_cable_prepare(a->cable, camera, p->root.xyz, p->phase == BFM_VILLAGE_RIDE,
                                  a->tiles, &pixels[0][0])) return 0;
    if (!bfm_village_gate_prepare(a->gate, camera, a->tiles, &pixels[0][0]) ||
        !bfm_village_upper_landmarks_prepare(a->signs,camera,a->tiles,&pixels[0][0])) return 0;
    if (a->guard && !bfm_village_upper_guard_add(a->guard,a->view.hero,camera)) return 0;
    if (!bfm_village_rotor_append(&a->rotor, a->view.hero, camera)) return 0;
    if (!bfm_village_leno_delivery_add(a->delivery, a->view.hero, camera)) return 0;
    if (!bfm_village_earth_return_add(a->earth_return, a->view.hero, camera)) return 0;
    if (a->residents && !bfm_village_residents_add(a->residents, a->view.hero, camera, a->age))
        return 0;
    a->surface = (BfmForestRaster){.tiles=a->tiles, .pixels=pixels, .top=10, .bottom=229};
    a->next_card = 0;
    a->card_count = bfm_village_impact_project(a->impact, camera, a->cards);
    qsort(a->cards, a->card_count, sizeof *a->cards, far_card_first);
    bfm_bedroom_field_draw(a->view.field, a->tiles, &pixels[0][0],
                           draw_actors, a);
    if (window && bfm_village_earth_return_active(a->earth_return)) {
        return bfm_village_earth_return_draw_window(a->earth_return, camera, window, pixels);
    }
    if (window && a->talking_to_guard)
        return bfm_village_upper_guard_draw_message(a->guard,camera,p->root.xyz,window,pixels);
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

/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_rescue.h"
#include "village_mine_lake_resident_art.h"
#include "village_mine_lake_audio.h"
#include "village_talk_camera.h"
#include "woods_fusion_art.h"
#include "woods_lardwick_award.h"
#include "woods_lardwick_motes.h"
#include "woods_lardwick_player.h"
#include "tower_summit_departure.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { BRISKET = 21, DICEY = 27, PLACEMENT_BYTES = 20, RING_BYTES = 42 };

/* Selectors in the mounted resident command format. Each command has one
 * 32-bit operand. Its type depends on the selector. */
typedef enum {
    SHOW_TEXT = 0, END_TALK = 2, WALK_RELATIVE = 4, FACE_RESIDENT = 10,
    RELEASE_CAMERA = 32, FRAME_CAMERA = 33, PLAY_CLIP = 35,
    START_EFFECT = 51, CALL_AWARD = 53, PLACE_WINDOW = 57,
    WAIT_FIELDS = 63, WAIT_PLAYER = 64, PLAY_SOUND = 65
} ResidentCommand;

typedef struct {
    ResidentCommand command;
    uint32_t argument;
    const uint8_t *data;
    size_t bytes;
} ResidentOrder;

typedef struct {
    ResidentOrder *orders;
    size_t count;
    const uint8_t *profile, *program;
    uint32_t departure_clip;
    BfmWoodsLardwickAward award;
} Resident;

typedef enum { IDLE, RUNNING, PLAYER_BARRIER, TIMER, MESSAGE, AWARD, RETURN } Phase;

struct BfmMineLakeRescue {
    const uint8_t *scene, *ring;
    size_t bytes;
    BfmSaveRecord *save;
    Resident residents[2], *active;
    BfmMineLakeResidentArt *art;
    BfmBedroomRoot root;
    BfmVillageTalkCamera camera;
    BfmWoodsLardwickPlayer player;
    BfmWoodsLardwickMotes motes;
    Phase phase;
    BfmMineLakeRescueWindow window;
    size_t cursor;
    unsigned pose_tick, effect_tick, wait;
    int departure_clip, effect_active, resident_gone, place_window;
};

static const uint8_t *mounted(const BfmMineLakeRescue *r, uint32_t address, size_t length)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base || address - base > r->bytes || length > r->bytes - (address - base))
        return NULL;
    return r->scene + (address - base);
}

/* The mounted profile immediately following the local program bounds its
 * extent. Decode before admitting a resident or mutating the live save. */
static int read_program(BfmMineLakeRescue *r, const uint8_t *profile,
                        const uint8_t *main, size_t main_bytes)
{
    unsigned id = profile[0];
    if ((id != BRISKET && id != DICEY) || profile[1] != 1) return 0;
    Resident *resident = &r->residents[id == DICEY];
    uint32_t entry = u32le(profile + 12);
    const uint8_t *program = mounted(r, entry, 8);
    if (resident->orders || (entry & 3u) || !program || program >= profile) return 0;
    size_t length = (size_t)(profile - program);
    if (length % 8) return 0;
    size_t count = length / 8;
    if (count > SIZE_MAX / sizeof *resident->orders) return 0;
    resident->orders = calloc(count, sizeof *resident->orders);
    if (!resident->orders) return 0;
    resident->profile = profile;
    resident->program = program;
    resident->count = count;

    int has_award = 0, has_text = 0, camera_held = 0, actor_sound = 0;
    unsigned pending_orders = 0;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t *record = program + i * 8;
        ResidentOrder *order = &resident->orders[i];
        uint32_t command = u32le(record);
        uint32_t arg = u32le(record + 4);
        order->argument = arg;
        switch (command) {
        case SHOW_TEXT:
            order->data = mounted(r, arg, 1);
            if (!order->data) return 0;
            has_text = 1;
            actor_sound = 0;
            break;
        case END_TALK:
            if (arg || i + 1 != count || camera_held || pending_orders) return 0;
            break;
        case WALK_RELATIVE:
        case FACE_RESIDENT:
            if (command == FACE_RESIDENT && arg) return 0;
            if (++pending_orders > 2) return 0;
            break;
        case WAIT_PLAYER:
            pending_orders = 0;
            if (arg) return 0;
            break;
        case RELEASE_CAMERA:
            if (arg) return 0;
            camera_held = 0;
            break;
        case PLACE_WINDOW:
            if (arg) return 0;
            break;
        case FRAME_CAMERA:
            order->data = mounted(r, arg, 16);
            if ((arg & 1u) || !order->data) return 0;
            camera_held = 1;
            break;
        case PLAY_CLIP:
            order->data = mounted(r, arg, 8);
            if ((arg & 3u) || !order->data ||
                (resident->departure_clip && resident->departure_clip != arg)) return 0;
            resident->departure_clip = arg;
            break;
        case START_EFFECT:
            /* Only the lake resident's departure callback has a host handler. */
            if (arg != UINT32_C(0x8017ccd4)) return 0;
            break;
        case CALL_AWARD:
            order->data = mounted(r, arg, 8);
            if ((arg & 3u) || !order->data || has_award ||
                !bfm_mine_lake_award_open(&resident->award, r->scene, r->bytes,
                                          main, main_bytes, arg, id)) return 0;
            has_award = 1;
            actor_sound = 0;
            break;
        case WAIT_FIELDS:
            if (arg) actor_sound = 0;
            break;
        case PLAY_SOUND:
            if (actor_sound || !arg || arg > UINT16_MAX || !bfm_mine_lake_rescue_cue(arg)) return 0;
            actor_sound = 1;
            break;
        default: return 0;
        }
        order->command = (ResidentCommand)command;
    }
    return resident->orders[count - 1].command == END_TALK && has_award &&
           has_text && resident->departure_clip;
}

static int by_location(const void *a, const void *b)
{
    const uint8_t *left = *(const uint8_t *const *)a;
    const uint8_t *right = *(const uint8_t *const *)b;
    return (left > right) - (left < right);
}

/* Bound each page at the next referenced scene object. The message player
 * interprets its control operands and end token within that span. */
static int bound_messages(BfmMineLakeRescue *r)
{
    size_t limit = r->residents[0].count + r->residents[1].count + 8;
    const uint8_t **objects = malloc(limit * sizeof *objects);
    if (!objects) return 0;
    size_t count = 0;
    objects[count++] = r->scene + r->bytes;
    objects[count++] = r->ring;
    for (unsigned i = 0; i < 2; ++i) {
        const Resident *resident = &r->residents[i];
        objects[count++] = resident->program;
        objects[count++] = resident->profile;
        const uint8_t *idle = mounted(r, u32le(resident->profile + 8), 8);
        if (!idle) { free(objects); return 0; }
        objects[count++] = idle;
        for (size_t k = 0; k < resident->count; ++k)
            if (resident->orders[k].data) objects[count++] = resident->orders[k].data;
    }
    qsort(objects, count, sizeof *objects, by_location);
    for (unsigned i = 0; i < 2; ++i) {
        Resident *resident = &r->residents[i];
        for (size_t k = 0; k < resident->count; ++k) {
            ResidentOrder *order = &resident->orders[k];
            if (order->command != SHOW_TEXT) continue;
            size_t lo = 0, hi = count;
            while (lo < hi) {
                size_t mid = lo + (hi - lo) / 2;
                if (objects[mid] <= order->data) lo = mid + 1;
                else hi = mid;
            }
            /* The end of the scene sorts after every validated data address. */
            order->bytes = (size_t)(objects[lo] - order->data);
        }
    }
    free(objects);
    return 1;
}

BfmMineLakeRescue *bfm_mine_lake_rescue_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, BfmSaveRecord *save)
{
    if (!scene || !main || !save || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmMineLakeRescue *r = calloc(1, sizeof *r);
    if (!r) return NULL;
    r->scene = scene;
    r->bytes = bytes;
    r->save = save;
    const uint8_t *room = mounted(r, UINT32_C(0x80185a50), 0xbc);
    r->ring = mounted(r, UINT32_C(0x80185894), RING_BYTES);
    if (!room || !r->ring) goto fail;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % PLACEMENT_BYTES) goto fail;
    const uint8_t *rows = mounted(r, first, end - first);
    if (!rows) goto fail;
    for (size_t i = 0; i < end - first; i += PLACEMENT_BYTES) {
        unsigned kind = u16le(rows + i + 6);
        if (!kind) {
            if (r->residents[0].orders && r->residents[1].orders && bound_messages(r)) return r;
            break;
        }
        if (kind != 296) continue;
        const uint8_t *profile = mounted(r, u32le(rows + i + 16), 16);
        if (!profile || !read_program(r, profile, main, main_bytes)) break;
    }
fail:
    bfm_mine_lake_rescue_free(r);
    return NULL;
}

void bfm_mine_lake_rescue_free(BfmMineLakeRescue *r)
{
    if (!r) return;
    free(r->residents[0].orders);
    free(r->residents[1].orders);
    bfm_mine_lake_resident_art_free(r->art);
    free(r);
}

static int frame_resident(BfmMineLakeRescue *r, const uint8_t *camera,
                          const BfmVillageCamera *presented)
{
    uint8_t station[8];
    for (unsigned i = 0; i < 4; ++i) {
        uint16_t value = i < 3 ? (uint16_t)r->root.xyz[i] : (uint16_t)r->root.yaw;
        station[2*i] = (uint8_t)value;
        station[2*i+1] = (uint8_t)(value >> 8);
    }
    return bfm_village_talk_camera_frame(&r->camera, station, camera, presented);
}

/* Run immediate commands together until a field service takes ownership.
 * The approach edge is after player movement, so any orders it issues are
 * sampled now and first serviced on the following player update. */
static int advance(BfmMineLakeRescue *r, const BfmVillageCamera *presented,
                   BfmVillageMotion *motion, BfmForestMessage **opened, BfmMineLakeRescueEvents *events)
{
    Resident *resident = r->active;
    while (r->cursor < resident->count) {
        const ResidentOrder *order = &resident->orders[r->cursor];
        switch (order->command) {
        case SHOW_TEXT:
            if (!opened) return 1;
            if (*opened || !(*opened = bfm_field_message_borrow(order->data, order->bytes))) return 0;
            r->window = BFM_LAKE_RESCUE_WINDOW_RESIDENT;
            r->phase = MESSAGE;
            break;
        case END_TALK:
            r->phase = RETURN;
            break;
        case FRAME_CAMERA:
            if (!frame_resident(r, order->data, presented)) return 0;
            break;
        case RELEASE_CAMERA:
            bfm_village_talk_camera_release(&r->camera);
            break;
        case WALK_RELATIVE:
        case FACE_RESIDENT: {
            BfmForestTransform actor;
            bfm_forest_actor_root(r->root.yaw, r->root.xyz, &actor);
            if (!bfm_woods_lardwick_player_queue(&r->player, order->command, order->argument,
                                                &actor, r->root.xyz)) return 0;
            break;
        }
        case PLAY_CLIP:
            r->departure_clip = 1;
            r->pose_tick = 0;
            break;
        case START_EFFECT:
            r->effect_active = 1;
            r->effect_tick = 0;
            break;
        case CALL_AWARD:
            if (!motion) return 1;
            if (!bfm_woods_lardwick_award_begin(&resident->award, motion)) return 0;
            r->resident_gone = 1;
            r->wait = resident->award.wait_fields;
            r->phase = AWARD;
            break;
        case PLACE_WINDOW:
            r->place_window = 1;
            break;
        case WAIT_FIELDS:
            r->wait = order->argument;
            if (r->wait) r->phase = TIMER;
            break;
        case WAIT_PLAYER:
            if (!r->player.ready) r->phase = PLAYER_BARRIER;
            break;
        case PLAY_SOUND:
            if (!events) return 1;
            if (events->actor_sound) return 0;
            events->actor_sound = order->argument;
            break;
        }
        ++r->cursor;
        if (r->phase != RUNNING) return 1;
    }
    return 0;
}

int bfm_mine_lake_rescue_begin(BfmMineLakeRescue *r, const BfmGameplayRun *run,
    unsigned id, const BfmBedroomRoot *root, const MusashiForestPlayerMesh *model,
    const uint8_t *resource, size_t resource_bytes, unsigned pose_tick,
    const BfmVillageCamera *presented)
{
    if (!r || !run || !root || !presented || r->phase != IDLE || (id != BRISKET && id != DICEY) ||
        r->save->variables[58+id] || run->banks.variables[58+id]) return 0;
    Resident *resident = &r->residents[id == DICEY];
    r->art = bfm_mine_lake_resident_art_load(r->scene, r->bytes, id, model,
        resource, resource_bytes, u32le(resident->profile + 4), u32le(resident->profile + 8),
        resident->departure_clip);
    if (!r->art) return 0;
    r->root = *root;
    r->active = resident;
    r->cursor = 0;
    r->pose_tick = pose_tick;
    r->effect_tick = 0;
    r->departure_clip = r->effect_active = r->resident_gone = r->place_window = 0;
    r->player = (BfmWoodsLardwickPlayer){0};
    r->motes = (BfmWoodsLardwickMotes){0};
    r->phase = RUNNING;
    if (advance(r, presented, NULL, NULL, NULL)) return 1;
    bfm_mine_lake_resident_art_free(r->art);
    r->art = NULL;
    r->active = NULL;
    r->camera = (BfmVillageTalkCamera){0};
    r->phase = IDLE;
    return 0;
}

int bfm_mine_lake_rescue_busy(const BfmMineLakeRescue *r)
{
    return r && r->phase != IDLE;
}

int bfm_mine_lake_rescue_drives_player(const BfmMineLakeRescue *r)
{
    return r && (r->player.count || r->phase == AWARD);
}

BfmMineLakeRescueWindow bfm_mine_lake_rescue_window(const BfmMineLakeRescue *r,
                                                    const BfmBedroomRoot **speaker)
{
    if (speaker) *speaker = r && r->place_window && r->window == BFM_LAKE_RESCUE_WINDOW_RESIDENT ? &r->root : NULL;
    return r ? r->window : BFM_LAKE_RESCUE_WINDOW_NONE;
}

int bfm_mine_lake_rescue_close(BfmMineLakeRescue *r)
{
    if (!r || !r->window) return 0;
    r->window = BFM_LAKE_RESCUE_WINDOW_NONE;
    return 1;
}

void bfm_mine_lake_rescue_camera(BfmMineLakeRescue *r, BfmVillageCamera *normal)
{
    if (r) bfm_village_talk_camera_step(&r->camera, normal);
}

int bfm_mine_lake_rescue_step(BfmMineLakeRescue *r, BfmGameplayRun *run, BfmVillageMotion *motion,
    const BfmVillageCamera *presented, const BfmBedroomView *view, int player_ready,
    BfmForestMessage **opened, BfmMineLakeRescueEvents *events)
{
    if (!r || !events) return 0;
    *events = (BfmMineLakeRescueEvents){0};
    if (r->phase == IDLE) return 1;
    if (!run || !motion || !presented || !view || !opened) return 0;
    if (!r->resident_gone) {
        ++r->pose_tick;
        if (r->effect_active) {
            ++r->effect_tick;
            bfm_woods_lardwick_motes_step(&r->motes, run, r->root.xyz, r->effect_tick);
        }
    }
    if (r->player.count || r->phase == PLAYER_BARRIER) {
        int yaw = bfm_village_motion_view_yaw(presented->eye, presented->target);
        if (!bfm_woods_lardwick_player_step(&r->player, motion, yaw, player_ready)) return 0;
    }
    switch (r->phase) {
    case PLAYER_BARRIER:
        if (!r->player.ready) return 1;
        break;
    case TIMER:
        if (--r->wait) return 1;
        break;
    case MESSAGE:
        if (r->window) return 1;
        break;
    case AWARD: {
        BfmWoodsLardwickAward *award = &r->active->award;
        BfmLardwickAwardFrame frame;
        if (!bfm_woods_lardwick_award_step(award, motion, view, !r->window, &frame)) return 0;
        events->player_sound = frame.sound;
        if (frame.open_notice) {
            if (*opened || !(*opened = bfm_woods_lardwick_award_message(award))) return 0;
            r->window = BFM_LAKE_RESCUE_WINDOW_AWARD;
        }
        if (r->wait && !--r->wait) events->actor_sound = bfm_woods_lardwick_award_bp(award, r->save, run);
        if (r->wait || !frame.complete || r->window) return 1;
        break;
    }
    case RETURN:
        if (r->camera.phase) return 1;
        if (!bfm_woods_lardwick_award_commit(&r->active->award, r->save, run)) return 0;
        bfm_mine_lake_resident_art_free(r->art);
        r->art = NULL;
        r->active = NULL;
        r->phase = IDLE;
        return 1;
    case RUNNING: break;
    default: return 0;
    }
    r->phase = RUNNING;
    return advance(r, presented, motion, opened, events);
}

int bfm_mine_lake_rescue_add(const BfmMineLakeRescue *r, BfmBedroomFigure *figure,
    const BfmBedroomView *view, unsigned tick)
{
    if (!r || !figure || !view) return 0;
    if (r->phase == IDLE || r->resident_gone) return 1;
    int leaving = r->effect_active;
    if ((!leaving || r->effect_tick <= 78) &&
        !bfm_mine_lake_resident_art_draw(r->art, &r->root, r->departure_clip, r->pose_tick,
                                         leaving, r->effect_tick, figure, view)) return 0;
    if (!leaving) return 1;
    BfmForestCamera camera;
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_TOWER_SUMMIT_DEPARTURE_CARDS];
    size_t count = bfm_departure_ring_cards(r->ring, RING_BYTES, r->root.xyz, r->effect_tick, tick,
                                            &camera, cards, BFM_TOWER_SUMMIT_DEPARTURE_CARDS);
    return (!count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count)) &&
           bfm_woods_lardwick_motes_add(&r->motes, figure, view);
}

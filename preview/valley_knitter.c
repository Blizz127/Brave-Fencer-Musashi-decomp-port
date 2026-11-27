/* SPDX-License-Identifier: MIT */
#include "valley_knitter.h"
#include "tower_summit_departure.h"
#include "village_talk_camera.h"
#include "woods_fusion_art.h"
#include "woods_lardwick_award.h"
#include "woods_lardwick_motes.h"
#include "woods_lardwick_player.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { KNITTER_ID = 29, RESCUE_VAR = 58 + KNITTER_ID, PARTS = 7, ALL_RESCUED = 35,
       FINAL_FIGURE = 37, MAX_BP = 1634 };
static const uint32_t room_base = UINT32_C(0x80128158), main_base = UINT32_C(0x800cedf8);

typedef enum {
    SEALED, UNCOVER, POSITION, GREETING, TALK, BEAM, DEPART, AWARD, RETURN, FINISHED
} RescuePhase;

typedef struct {
    unsigned count;
    const uint8_t *keys[20];
} BodyClip;

struct BfmValleyKnitter {
    BfmSaveRecord *save;
    const uint8_t *script, *camera_record, *message, *ring;
    size_t message_bytes;
    MusashiForestPlayerMesh *mesh;
    BodyClip standing, leaving;
    BfmBedroomRoot root;
    BfmWoodsLardwickPlayer player;
    BfmWoodsLardwickAward award;
    BfmWoodsLardwickMotes motes;
    BfmVillageTalkCamera camera;
    RescuePhase phase;
    BfmValleyKnitterWindow window;
    unsigned wait, age;
};

static const uint8_t *mounted(const uint8_t *data, size_t size, uint32_t base,
                              uint32_t address, size_t length)
{
    if (!data || address < base || address - base > size ||
        length > size - (address - base)) return NULL;
    return data + (address - base);
}

static const uint8_t *room_data(const uint8_t *scene, size_t size, uint32_t address, size_t length)
{
    return mounted(scene, size, room_base, address, length);
}

static uint32_t operand(const uint8_t *script, unsigned row)
{
    return u32le(script + 8 * row + 4);
}

static void put16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static const uint8_t *captive_profile(const uint8_t *scene, size_t bytes)
{
    /* Verify this member's room binding and look up the actual Bincho placement. */
    const uint8_t *binding = room_data(scene, bytes, UINT32_C(0x8018231c), 4);
    if (!binding || u32le(binding) != UINT32_C(0x80186620)) return NULL;
    const uint8_t *room = room_data(scene, bytes, u32le(binding), 0xbc);
    if (!room) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (first > end || (end - first) % 20) return NULL;
    const uint8_t *rows = room_data(scene, bytes, first, end - first), *found = NULL;
    if (!rows) return NULL;
    for (size_t i = 0; i < end - first; i += 20) {
        unsigned kind = u16le(rows + i + 6);
        if (!kind) return found;
        if (kind != 296) continue;
        const uint8_t *p = room_data(scene, bytes, u32le(rows + i + 16), 16);
        if (!p) return NULL;
        if (p[0] != KNITTER_ID) continue;
        if (found || p[1]) return NULL;
        found = p;
    }
    return NULL;
}

static int load_clip(BodyClip *clip, const uint8_t *scene, size_t size, uint32_t address,
                     unsigned count)
{
    const uint8_t *rows = room_data(scene, size, address, (count + 1) * 8);
    if (!rows || count > sizeof clip->keys / sizeof clip->keys[0] ||
        u32le(rows + count * 8) || operand(rows, count) != 0xffff) return 0;
    for (unsigned i = 0; i < count; ++i) {
        uint32_t pose = u32le(rows + 8 * i);
        if ((pose >> 24) != 0x81 || (pose & 3) || operand(rows, i) != 1) return 0;
        clip->keys[i] = room_data(scene, size, pose & ~UINT32_C(0x01000000), PARTS * 8);
        if (!clip->keys[i]) return 0;
    }
    clip->count = count;
    return 1;
}

static int load_award(BfmWoodsLardwickAward *a, const uint8_t *scene, size_t size,
                      uint32_t address, const uint8_t *main, size_t main_size)
{
    const uint8_t *rows = room_data(scene, size, address, 40);
    if (!rows || u32le(rows) != 59 || u32le(rows + 8) != 63 || !operand(rows, 1) ||
        u32le(rows + 16) != 51 || operand(rows, 2) != UINT32_C(0x8017cbb8) ||
        u32le(rows + 24) != 64 || u32le(rows + 32) != 2) return 0;
    const uint8_t *order = room_data(scene, size, operand(rows, 0), 28);
    if (!order) return 0;
    for (unsigned i = 0; i < 24; i += 4)
        if (u32le(order + i)) return 0;
    const uint8_t *msg = mounted(main, main_size, main_base, u32le(order + 24), 4);
    if (!msg || msg[0] != 2 || msg[1] != 1 || msg[2] != 0) return 0;
    const uint8_t *end = memchr(msg + 3, 0, main_size - (size_t)(msg + 3 - main));
    const uint8_t *offer = mounted(main, main_size, main_base,
                                   UINT32_C(0x8010f46c) + 8 * FINAL_FIGURE, 3);
    if (!end || !offer) return 0;
    a->message = msg;
    a->message_bytes = (size_t)(end - msg) + 1;
    a->wait_fields = operand(rows, 1);
    a->figure_price = u16le(offer);
    a->figure_trend = offer[2];
    return 1;
}

static int load_captive(BfmValleyKnitter *k, const uint8_t *scene, size_t size,
                        const uint8_t *profile, const uint8_t *main, size_t main_size)
{
    const uint8_t *s = room_data(scene, size, u32le(profile + 12), 16 * 8);
    if (!s || u32le(s) != 33 || u32le(s + 8) != 4 || u32le(s + 16) != 10 ||
        u32le(s + 24) != 64 || u32le(s + 32) != 63 || u32le(s + 40) != 57 ||
        u32le(s + 48) || u32le(s + 56) != 35 || u32le(s + 64) != 51 ||
        operand(s, 8) != UINT32_C(0x8017cf98) || u32le(s + 72) != 65 ||
        u32le(s + 80) != 63 || u32le(s + 88) != 65 || u32le(s + 96) != 63 ||
        u32le(s + 104) != 53 || u32le(s + 112) != 32 || u32le(s + 120) != 2) return 0;
    k->script = s;
    k->camera_record = room_data(scene, size, operand(s, 0), 16);
    k->ring = room_data(scene, size, UINT32_C(0x80185ea4), 42);
    k->message = room_data(scene, size, operand(s, 6), 4);
    if (!k->message || k->message[0] != 5 || k->message[1] != 2 ||
        k->message[2] != 10 || k->message[3] != 13) return 0;
    const uint8_t *end = memchr(k->message + 4, 0, size - (size_t)(k->message + 4 - scene));
    if (!end) return 0;
    k->message_bytes = (size_t)(end - k->message) + 1;
    unsigned parts;
    k->mesh = musashi_forest_player_mesh_from_table(scene, size, room_base, u32le(profile + 4), &parts);
    return k->camera_record && k->ring && k->mesh && parts == PARTS &&
           load_clip(&k->standing, scene, size, u32le(profile + 8), 10) &&
           load_clip(&k->leaving, scene, size, operand(s, 7), 20) &&
           load_award(&k->award, scene, size, operand(s, 13), main, main_size);
}

BfmValleyKnitter *bfm_valley_knitter_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, BfmSaveRecord *save)
{
    const uint8_t *profile = captive_profile(scene, bytes);
    if (!profile || !save || !main) return NULL;
    BfmValleyKnitter *k = calloc(1, sizeof *k);
    if (!k) return NULL;
    k->save = save;
    if (save->variables[RESCUE_VAR]) k->phase = FINISHED;
    else if (!load_captive(k, scene, bytes, profile, main, main_bytes)) {
        bfm_valley_knitter_free(k);
        return NULL;
    }
    return k;
}

void bfm_valley_knitter_free(BfmValleyKnitter *k)
{
    if (k) musashi_forest_player_mesh_free(k->mesh);
    free(k);
}

int bfm_valley_knitter_busy(const BfmValleyKnitter *k)
{
    return k && k->phase != SEALED && k->phase != FINISHED;
}

int bfm_valley_knitter_drives_player(const BfmValleyKnitter *k)
{
    return k && ((k->phase == POSITION && k->player.count) || k->phase == AWARD);
}

BfmValleyKnitterWindow bfm_valley_knitter_window(const BfmValleyKnitter *k,
                                                 const BfmBedroomRoot **speaker)
{
    if (speaker) *speaker = k && k->window == BFM_KNITTER_WINDOW_CAPTIVE ? &k->root : NULL;
    return k ? k->window : BFM_KNITTER_WINDOW_NONE;
}

int bfm_valley_knitter_close(BfmValleyKnitter *k)
{
    if (!k || !k->window) return 0;
    k->window = BFM_KNITTER_WINDOW_NONE;
    return 1;
}

void bfm_valley_knitter_camera(BfmValleyKnitter *k, BfmVillageCamera *camera)
{
    if (k) bfm_village_talk_camera_step(&k->camera, camera);
}

static int approach_player(BfmValleyKnitter *k, const BfmVillageCamera *presented)
{
    uint8_t station[8];
    for (unsigned axis = 0; axis < 3; ++axis)
        put16(station + axis * 2, (uint16_t)k->root.xyz[axis]);
    put16(station + 6, (uint16_t)k->root.yaw);
    BfmForestTransform model;
    bfm_forest_actor_root(k->root.yaw, k->root.xyz, &model);
    return bfm_village_talk_camera_frame(&k->camera, station, k->camera_record, presented) &&
           bfm_woods_lardwick_player_queue(&k->player, 4, operand(k->script, 1), &model, k->root.xyz) &&
           bfm_woods_lardwick_player_queue(&k->player, 10, 0, &model, k->root.xyz);
}

static void grant_bp(BfmValleyKnitter *k, BfmGameplayRun *run)
{
    uint8_t *p = run->has_player ? run->player.record : k->save->progress;
    uint16_t max = (uint16_t)(u16le(p + 0x3e) + 5);
    uint16_t refill = (uint16_t)(u16le(p + 0x40) + 9999);
    if (max > MAX_BP) max = MAX_BP;
    put16(p + 0x3e, max);
    put16(p + 0x40, refill > max ? max : refill);
    if (p != k->save->progress) memcpy(k->save->progress + 0x3e, p + 0x3e, 4);
}

static void register_captive(BfmValleyKnitter *k, BfmGameplayRun *run)
{
    uint8_t *p = run->has_player ? run->player.record : k->save->progress;
    uint32_t count = u32le(p + 12) + 1u;
    k->save->variables[RESCUE_VAR] = run->banks.variables[RESCUE_VAR] = 1;
    for (unsigned i = 0; i < 4; ++i) p[12+i] = (uint8_t)(count >> (8*i));
    if (p != k->save->progress) memcpy(k->save->progress + 12, p + 12, 4);
    if (count >= ALL_RESCUED) {
        size_t stock = 98 + FINAL_FIGURE, price = 2 * (FINAL_FIGURE - 1);
        if (!(k->save->variables[stock] & 0x80u)) {
            k->save->variables[stock] = run->banks.variables[stock] = k->award.figure_trend | 0x80u;
            put16(k->save->figure_prices + price, k->award.figure_price);
            put16(run->banks.figure_prices + price, k->award.figure_price);
        }
    }
}

int bfm_valley_knitter_step(BfmValleyKnitter *k, BfmGameplayRun *run, BfmVillageMotion *motion,
    const BfmVillageCamera *presented, const BfmBedroomView *view, int player_ready,
    const BfmBedroomRoot *crystal, unsigned captive_yaw, int reveal, int approach,
    BfmForestMessage **opened, BfmValleyKnitterEvents *events)
{
    if (!k || !run || !opened || !events) return 0;
    *events = (BfmValleyKnitterEvents){0};
    if (k->phase == FINISHED) return 1;
    if (k->phase == SEALED) {
        if (k->save->variables[RESCUE_VAR] || run->banks.variables[RESCUE_VAR]) {
            k->phase = FINISHED;
            return 1;
        }
        if (!reveal) return !approach;
        if (!crystal) return 0;
        k->root = *crystal;
        k->root.yaw = captive_yaw & 4095u;
        k->phase = UNCOVER;
    } else if (k->phase <= DEPART) ++k->age;
    if (!motion || !presented || !view) return 0;
    switch (k->phase) {
    case UNCOVER:
        if (approach) {
            if (!approach_player(k, presented)) return 0;
            k->phase = POSITION;
        }
        break;
    case POSITION: {
        int bearing = bfm_village_motion_view_yaw(presented->eye, presented->target);
        if (!bfm_woods_lardwick_player_step(&k->player, motion, bearing, player_ready)) return 0;
        if (k->player.ready) {
            k->wait = operand(k->script, 4);
            k->phase = GREETING;
        }
        break;
    }
    case GREETING:
        if (k->wait && --k->wait) break;
        if (*opened || !(*opened = bfm_field_message_borrow(k->message, k->message_bytes))) return 0;
        k->window = BFM_KNITTER_WINDOW_CAPTIVE;
        k->phase = TALK;
        break;
    case TALK:
        if (k->window) break;
        k->age = 0;
        k->wait = operand(k->script, 10);
        events->actor_sound = operand(k->script, 9);
        k->phase = BEAM;
        break;
    case BEAM:
    case DEPART:
        bfm_woods_lardwick_motes_step(&k->motes, run, k->root.xyz, k->age);
        if (k->wait && --k->wait) break;
        if (k->phase == BEAM) {
            events->actor_sound = operand(k->script, 11);
            k->wait = operand(k->script, 12);
            k->phase = DEPART;
        } else {
            if (!bfm_woods_lardwick_award_begin(&k->award, motion)) return 0;
            k->wait = k->award.wait_fields;
            k->phase = AWARD;
        }
        break;
    case AWARD: {
        BfmLardwickAwardFrame frame;
        if (!bfm_woods_lardwick_award_step(&k->award, motion, view, !k->window, &frame)) return 0;
        events->player_sound = frame.sound;
        if (frame.open_notice) {
            if (*opened || !(*opened = bfm_woods_lardwick_award_message(&k->award))) return 0;
            k->window = BFM_KNITTER_WINDOW_AWARD;
        }
        if (k->wait && !--k->wait) {
            grant_bp(k, run);
            events->actor_sound = 3070;
        }
        if (!k->wait && frame.complete && !k->window) {
            bfm_village_talk_camera_release(&k->camera);
            k->phase = RETURN;
        }
        break;
    }
    case RETURN:
        if (!k->camera.phase) {
            register_captive(k, run);
            k->phase = FINISHED;
        }
        break;
    default: return 0;
    }
    return 1;
}

int bfm_valley_knitter_add(const BfmValleyKnitter *k, BfmBedroomFigure *figure,
                          const BfmBedroomView *view, unsigned tick)
{
    if (!k || !figure || !view) return 0;
    if (k->phase < UNCOVER || k->phase > DEPART) return 1;
    int leaving = k->phase == BEAM || k->phase == DEPART;
    if (!leaving || k->age <= 78) {
        const BodyClip *clip = leaving ? &k->leaving : &k->standing;
        BfmForestPose pose;
        if (!bfm_forest_actor_pose_decode(clip->keys[k->age % clip->count], 8*PARTS, PARTS, &pose))
            return 0;
        BfmForestTransform root;
        bfm_forest_actor_root(k->root.yaw, k->root.xyz, &root);
        if (leaving) bfm_tower_summit_departure_hero(k->age, &root);
        const BfmForestActor body = {.mesh=k->mesh, .root=&root, .pose=&pose,
                                     .visible_parts=(1u << PARTS)-1};
        if (!bfm_bedroom_figure_add_actor(figure, view, &body, NULL, NULL, 0)) return 0;
    }
    if (!leaving) return 1;
    BfmForestCamera camera;
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_TOWER_SUMMIT_DEPARTURE_CARDS];
    size_t count = bfm_departure_ring_cards(k->ring, 42, k->root.xyz, k->age, tick, &camera,
                                             cards, BFM_TOWER_SUMMIT_DEPARTURE_CARDS);
    return (!count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count)) &&
           bfm_woods_lardwick_motes_add(&k->motes, figure, view);
}

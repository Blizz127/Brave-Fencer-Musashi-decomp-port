/* SPDX-License-Identifier: MIT */
#include "woods_lardwick_field.h"
#include "../pc_port/forest_player_bytes.h"

#include "bedroom_effects.h"
#include "forest_bullet_art.h"
#include "village_arrival_motion.h"
#include <limits.h>
#include <string.h>

enum { SEALED, GLOW_BIRTH, GLOW_GROW, FRACTURING, REVEAL_READY, SUMMON, UNCOVER,
       APPROACH, POSITIONING, GREETING, SPEAKING, DEPARTURE, DISAPPEARING,
       AWARD, AWARD_WAIT, REGISTER, CLEANUP, FINISHED };

static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t room = UINT32_C(0x8018ea38);

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < scene_base) return NULL;
    size_t offset = address - scene_base;
    return offset <= bytes && count <= bytes - offset ? scene + offset : NULL;
}

static int mount_rescue(BfmWoodsLardwickField *field, const uint8_t *scene, size_t bytes,
                        const uint8_t *captive)
{
    const uint8_t *profile = mounted(scene, bytes, UINT32_C(0x8018b24c), 32);
    field->accepted_hits = mounted(scene, bytes, UINT32_C(0x8018b280), 8);
    if (!profile || !field->accepted_hits || !(u16le(profile + 2) & 0x1800)) return 0;
    const uint8_t *shape = mounted(scene, bytes, u32le(profile + 8), 16);
    if (!shape || u32le(shape) || captive[1]) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        field->bounds.low[axis] = s16le(shape + 4 + 4 * axis);
        field->bounds.high[axis] = s16le(shape + 6 + 4 * axis);
        if (field->bounds.low[axis] > field->bounds.high[axis]) return 0;
    }

    field->scene = scene;
    field->bytes = bytes;
    field->soldier_model = u32le(captive + 4);
    field->soldier_animation = u32le(captive + 8);
    field->script = mounted(scene, bytes, u32le(captive + 12), 16 * 8);
    if (!field->script) return 0;
    /* The actor starts a local conversation, changes animation, then calls the
     * shared Bincho award scene. Keep the localization and settings on disc. */
    if (u32le(field->script + 6 * 8) != 0 || u32le(field->script + 7 * 8) != 35 ||
        u32le(field->script + 13 * 8) != 53) return 0;
    field->soldier_talk_animation = u32le(field->script + 7 * 8 + 4);
    field->award_script = mounted(scene, bytes, u32le(field->script + 13 * 8 + 4), 5 * 8);
    field->glow_visible = field->crystal_visible = 1;
    return field->award_script && mounted(scene, bytes, field->soldier_model, 4) &&
           mounted(scene, bytes, field->soldier_animation, 8) &&
           mounted(scene, bytes, field->soldier_talk_animation, 8) &&
           mounted(scene, bytes, u32le(field->script + 4), 16) &&
           mounted(scene, bytes, u32le(field->script + 6 * 8 + 4), 1);
}

int bfm_woods_lardwick_field_open(BfmWoodsLardwickField *field, const uint8_t *scene,
                                 size_t bytes, const uint8_t variables[256])
{
    if (!field || !scene || !variables) return 0;
    const uint8_t *header = mounted(scene, bytes, room, 0xbc);
    if (!header) return 0;
    uint32_t first = u32le(header + 0xb4), end = u32le(header + 0xb8);
    if (end < first || (end - first) % 20) return 0;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return 0;

    BfmWoodsLardwickField result = {0};
    int found = 0;
    for (size_t i = 0; i < end - first; i += 20) {
        const uint8_t *row = rows + i;
        unsigned type = u16le(row + 6);
        if (!type) {
            if (!found) return 0;
            *field = result;
            return 1;
        }
        if (type != 296) continue;
        const uint8_t *captive = mounted(scene, bytes, u32le(row + 16), 16);
        if (!captive) return 0;
        if (captive[0] != 11) continue;
        if (found) return 0;
        found = 1;
        if (variables[58 + captive[0]]) continue;
        if (!mount_rescue(&result, scene, bytes, captive)) return 0;
        result.active = 1;
        for (unsigned axis = 0; axis < 3; ++axis) result.root.xyz[axis] = s16le(row + 2 * axis);
        if (result.root.xyz[1] < INT16_MIN + 4) return 0;
        result.root.xyz[1] -= 4;
        result.root.yaw = u16le(row + 12) & 4095u;
        result.root.normal[1] = -4096;
    }
    return 0;
}

static unsigned proximity_step(const int16_t from[3], const int16_t to[3])
{
    int64_t distance2 = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t d = (int)from[i] - to[i];
        distance2 += d * d;
    }
    /* The distance quantizer crosses each 256-unit boundary exactly. */
    for (int band = 0; band < 8; ++band) {
        int radius = (band + 1) * 256;
        if (distance2 < radius * radius) return 68u - 8u * (unsigned)band;
    }
    return 4;
}

static void light(BfmWoodsLardwickField *field, int flash)
{
    field->flash = flash;
    field->lights_active = 1;
    field->light_pitch = (field->light_pitch + 128) & 4095u;
    field->light_yaw = (field->light_yaw + 256) & 4095u;
}

void bfm_woods_lardwick_field_step(BfmWoodsLardwickField *field, BfmGameplayRun *run,
                                  const int16_t player[3], unsigned field_tick)
{
    if (!field || !field->active || field->phase != SEALED || !run || !player) return;
    if (!(field_tick & 15u)) field->detector_step = proximity_step(field->root.xyz, player);
    field->root.yaw = (field->root.yaw + 32) & 4095u;
    light(field, (bfm_gameplay_random(run) & 15u) == 0);
}

static int accepts(const BfmWoodsLardwickField *field, unsigned event)
{
    return field && field->active && field->phase == SEALED && event <= UINT8_MAX &&
           memchr(field->accepted_hits, (int)event, 8) != NULL;
}

int bfm_woods_lardwick_contact(const BfmWoodsLardwickField *field, unsigned event,
                               const int16_t from[3], const int16_t to[3], int16_t point[3],
                               int16_t normal[3])
{
    if (!from || !to || !point || !accepts(field, event)) return 0;
    BfmForestTransform model;
    int16_t center[3];
    bfm_forest_actor_root(field->root.yaw, field->root.xyz, &model);
    return bfm_forest_guard_contact_point(&field->bounds, &model, from, to, center, point, normal);
}

int bfm_woods_lardwick_strike(BfmWoodsLardwickField *field, unsigned event)
{
    if (!accepts(field, event)) return 0;
    field->phase = GLOW_BIRTH;
    return 1;
}

int bfm_woods_lardwick_busy(const BfmWoodsLardwickField *field)
{
    return field && field->active && field->phase != SEALED;
}

int bfm_woods_lardwick_controls_locked(const BfmWoodsLardwickField *field)
{
    return bfm_woods_lardwick_busy(field) && field->phase != CLEANUP;
}

BfmForestMessage *bfm_woods_lardwick_notice(BfmWoodsLardwickField *field)
{
    if (!field || field->phase != AWARD_WAIT || field->window) return NULL;
    BfmForestMessage *message = bfm_woods_lardwick_award_message(&field->award);
    if (message) field->window = BFM_LARDWICK_WINDOW_AWARD;
    return message;
}

int bfm_woods_lardwick_close(BfmWoodsLardwickField *field)
{
    if (!field || !field->window) return 0;
    field->window = BFM_LARDWICK_WINDOW_NONE;
    return 1;
}

static uint32_t operand(const BfmWoodsLardwickField *field, unsigned row)
{
    return u32le(field->script + row * 8 + 4);
}

BfmBinchoReveal bfm_bincho_reveal_step(BfmWoodsLardwickField *field, BfmGameplayRun *run,
    const int32_t eye[3], BfmWoodsLardwickEvents *events)
{
    if (!field || !run || !eye || !events) return BFM_BINCHO_REVEAL_INVALID;
    switch (field->phase) {
    case GLOW_BIRTH: {
        const int32_t anchor[3] = {field->root.xyz[0], field->root.xyz[1] - 56, field->root.xyz[2]};
        bfm_forest_glow_facing(eye, anchor, 5, &field->glow_transform);
        field->wait = 32;
        field->phase = GLOW_GROW;
        break;
    }
    case GLOW_GROW:
        field->glow_scale += 32;
        if (field->wait) {
            --field->wait;
            light(field, (bfm_gameplay_random(run) & 1u) == 0);
        } else {
            field->glow_visible = 0;
            field->detector_step = 4;
            events->sound = BFM_WOODS_CRYSTAL_BREAK_SOUND;
            field->wait = 40;
            field->phase = FRACTURING;
        }
        break;
    case FRACTURING:
        field->glow_scale += 128;
        if (field->wait) {
            --field->wait;
            ++field->burst_updates;
            light(field, 1);
        } else {
            field->crystal_visible = 0;
            field->phase = REVEAL_READY;
        }
        break;
    case REVEAL_READY:
        field->phase = SUMMON;
        break;
    case SUMMON: {
        const int32_t center[3] = {field->root.xyz[0], field->root.xyz[1], field->root.xyz[2]};
        field->soldier_yaw = (bfm_village_motion_view_yaw(center, eye) + 2048) & 4095u;
        field->soldier_visible = 1;
        field->phase = UNCOVER;
        return BFM_BINCHO_REVEAL_SPAWN;
    }
    case UNCOVER:
        if (field->glow_scale >= 192) field->glow_scale -= 192;
        else {
            field->glow_scale = 0;
            field->phase = APPROACH;
            return BFM_BINCHO_REVEAL_APPROACH;
        }
        break;
    default: return BFM_BINCHO_REVEAL_INVALID;
    }
    return BFM_BINCHO_REVEAL_RUNNING;
}

static int conversation(BfmWoodsLardwickField *field, int player_queue_idle,
                        BfmForestMessage **opened, BfmWoodsLardwickEvents *events)
{
    switch (field->phase) {
    case APPROACH: {
        uint32_t displacement = operand(field, 1);
        events->begin_approach = 1;
        events->camera = operand(field, 0);
        events->approach_x = (int16_t)(uint16_t)displacement;
        events->approach_z = (int16_t)(uint16_t)(displacement >> 16);
        field->phase = POSITIONING;
        break;
    }
    case POSITIONING:
        if (player_queue_idle) {
            field->wait = operand(field, 4);
            field->phase = GREETING;
        }
        break;
    case GREETING: {
        if (field->wait && --field->wait) break;
        uint32_t address = operand(field, 6);
        const uint8_t *text = mounted(field->scene, field->bytes, address, 1);
        if (!text || *opened) return 0;
        *opened = bfm_field_message_borrow(text, field->bytes - (address - scene_base));
        if (!*opened) return 0;
        field->window = BFM_LARDWICK_WINDOW_SOLDIER;
        field->phase = SPEAKING;
        break;
    }
    case SPEAKING:
        if (field->window) break;
        field->soldier_animation = field->soldier_talk_animation;
        field->soldier_pose_age = 0;
        events->depart = 1;
        events->sound = operand(field, 9);
        field->wait = operand(field, 10);
        field->phase = DEPARTURE;
        break;
    case DEPARTURE:
        if (field->wait && --field->wait) break;
        events->sound = operand(field, 11);
        field->wait = operand(field, 12);
        field->phase = DISAPPEARING;
        break;
    case DISAPPEARING:
        if (field->wait && --field->wait) break;
        field->soldier_visible = 0;
        field->phase = AWARD;
        break;
    default: return 0;
    }
    return 1;
}

int bfm_woods_lardwick_release_step(BfmWoodsLardwickField *field, BfmGameplayRun *run,
                                   BfmSaveRecord *save, const uint8_t *main, size_t main_bytes,
                                   const int32_t camera_eye[3], int player_queue_idle, int camera_idle,
                                   BfmForestMessage **opened, BfmWoodsLardwickEvents *events)
{
    if (!field || !run || !save || !camera_eye || !opened || !events) return 0;
    *events = (BfmWoodsLardwickEvents){0};
    if (!bfm_woods_lardwick_busy(field)) return 1;
    if (field->soldier_visible) ++field->soldier_pose_age;
    if (field->phase == DEPARTURE || field->phase == DISAPPEARING)
        bfm_woods_lardwick_motes_step(&field->motes, run, field->root.xyz, field->soldier_pose_age);
    if (field->phase <= UNCOVER) return bfm_bincho_reveal_step(field, run, camera_eye, events) != BFM_BINCHO_REVEAL_INVALID;
    if (field->phase <= DISAPPEARING) {
        if (!conversation(field, player_queue_idle, opened, events)) return 0;
        if (field->phase != AWARD) return 1;
    }

    switch (field->phase) {
    case AWARD:
        if (!bfm_woods_lardwick_award_open(&field->award, field->scene, field->bytes,
                                          main, main_bytes, operand(field, 13))) return 0;
        field->wait = field->award.wait_fields;
        events->award_begin = 1;
        field->phase = AWARD_WAIT;
        return 1;
    case AWARD_WAIT:
        if (field->wait && !--field->wait)
            events->sound = bfm_woods_lardwick_award_bp(&field->award, save, run);
        if (!field->wait && !field->window && player_queue_idle) {
            events->camera_release = 1;
            field->phase = REGISTER;
        }
        return 1;
    case REGISTER:
        /* The return cue starts easing the view on the previous actor pass.
         * The field script retains control until that camera is idle. */
        if (!camera_idle) return 1;
        if (!bfm_woods_lardwick_award_commit(&field->award, save, run)) return 0;
        field->phase = CLEANUP;
        return 1;
    case CLEANUP:
        field->active = 0;
        field->detector_step = 0;
        field->phase = FINISHED;
        return 1;
    default: return 0;
    }
}

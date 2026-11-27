/* SPDX-License-Identifier: MIT */
#include "valley_bincho.h"
#include "valley_hinge.h"
#include "woods_lardwick_field.h"
#include "woods_resident_glow_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t base = UINT32_C(0x80128158);
static const uint32_t profile_address = UINT32_C(0x80185d90);
enum { KNITTER = 29, FIELD_CLASS = 296 };

struct BfmValleyBincho {
    BfmWoodsLardwickField crystal;
    BfmWoodsResidentGlowArt *art;
    const uint8_t *registered;
    unsigned pending_hit;
    int uncovered;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (address < base || address - base > bytes || length > bytes - (address - base)) return NULL;
    return scene + (address - base);
}

BfmValleyBincho *bfm_valley_bincho_open(const uint8_t *scene, size_t bytes,
                                        const uint8_t variables[256])
{
    if (!scene || !variables || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    const uint8_t *room = mounted(scene, bytes, UINT32_C(0x80186620), 0xbc);
    if (!room) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % 20) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return NULL;

    const uint8_t *placement = NULL;
    size_t offset;
    for (offset = 0; offset < end - first; offset += 20) {
        const uint8_t *row = rows + offset;
        unsigned type = u16le(row + 6);
        if (!type) break;
        if (type != FIELD_CLASS) continue;
        const uint8_t *captive = mounted(scene, bytes, u32le(row + 16), 16);
        if (!captive) return NULL;
        if (captive[0] != KNITTER) continue;
        if (placement || captive[1] || s16le(row + 2) < INT16_MIN + 4) return NULL;
        placement = row;
    }
    if (!placement || offset == end - first) return NULL;
    BfmValleyBincho *actor = calloc(1, sizeof *actor);
    if (!actor) return NULL;
    actor->registered = variables + 58 + KNITTER;
    if (*actor->registered) return actor;

    const uint8_t *profile = mounted(scene, bytes, profile_address, 12);
    if (!profile || !(u16le(profile + 2) & 0x1800)) goto fail;
    const uint8_t *shape = mounted(scene, bytes, u32le(profile + 8), 16);
    BfmWoodsLardwickField *field = &actor->crystal;
    field->accepted_hits = mounted(scene, bytes, UINT32_C(0x80185dc4), 8);
    if (!shape || u32le(shape) || !field->accepted_hits) goto fail;
    for (unsigned axis = 0; axis < 3; ++axis) {
        field->bounds.low[axis] = s16le(shape + 4 + 4 * axis);
        field->bounds.high[axis] = s16le(shape + 6 + 4 * axis);
        if (field->bounds.high[axis] < field->bounds.low[axis]) goto fail;
        field->root.xyz[axis] = s16le(placement + 2 * axis);
    }
    field->root.xyz[1] -= 4;
    field->root.yaw = u16le(placement + 12) & 4095;
    field->root.normal[1] = -4096;
    field->active = field->glow_visible = field->crystal_visible = 1;
    actor->art = bfm_bincho_glow_art_load(scene, bytes, profile_address, UINT32_C(0x801c0814));
    if (actor->art) return actor;
fail:
    bfm_valley_bincho_free(actor);
    return NULL;
}

void bfm_valley_bincho_free(BfmValleyBincho *actor)
{
    if (actor) bfm_woods_resident_glow_art_free(actor->art);
    free(actor);
}

const BfmBedroomRoot *bfm_valley_bincho_root(const BfmValleyBincho *actor)
{
    return actor && actor->crystal.active ? &actor->crystal.root : NULL;
}

int bfm_valley_bincho_detector(const BfmValleyBincho *actor)
{
    return actor && actor->crystal.active && !*actor->registered ? (int)actor->crystal.detector_step : -1;
}

int bfm_valley_bincho_releasing(const BfmValleyBincho *actor)
{
    return actor && !actor->uncovered &&
           (actor->pending_hit || bfm_woods_lardwick_busy(&actor->crystal));
}

int bfm_valley_bincho_body(const BfmValleyBincho *actor, const int16_t previous[3],
                          int16_t proposed[3], unsigned tick, int airborne)
{
    if (!actor || !actor->crystal.crystal_visible || *actor->registered) return 0;
    const BfmWoodsLardwickField *field = &actor->crystal;
    BfmForestTransform body;
    bfm_forest_actor_root(field->root.yaw, field->root.xyz, &body);
    return bfm_valley_box_block_body(&field->bounds, &body, previous, proposed, tick, airborne);
}

int bfm_valley_bincho_hit(BfmValleyBincho *actor, const BfmVillageGround *ground,
                          const BfmWoodsCasterMeleeFrame *swing)
{
    if (!actor || !ground || !swing || actor->pending_hit || !actor->crystal.active || *actor->registered ||
        swing->rays > sizeof swing->to / sizeof swing->to[0]) return 0;
    for (unsigned i = 0; i < swing->rays; ++i) {
        int16_t end[3], normal[3], hit[3];
        uint16_t material;
        memcpy(end, swing->to[i], sizeof end);
        bfm_village_ground_blade(ground, swing->from, end, normal, &material);
        if (bfm_woods_lardwick_contact(&actor->crystal, swing->event, swing->from, end, hit, NULL)) {
            actor->pending_hit = swing->event;
            return 1;
        }
    }
    return 0;
}

int bfm_valley_bincho_step(BfmValleyBincho *actor, BfmGameplayRun *run, const int16_t player[3],
    const int32_t eye[3], unsigned tick, BfmValleyBinchoEvents *events)
{
    if (!actor || !run || !player || !eye || !events) return 0;
    *events = (BfmValleyBinchoEvents){0};
    if (!actor->crystal.active || *actor->registered || actor->uncovered) return 1;
    BfmWoodsLardwickField *field = &actor->crystal;
    if (!bfm_woods_lardwick_busy(field)) {
        bfm_woods_lardwick_field_step(field, run, player, tick);
        /* The rotating model and its random flash get one final sealed update
         * before the crystal consumes the player's posted attack. */
        if (actor->pending_hit) {
            bfm_woods_lardwick_strike(field, actor->pending_hit);
            actor->pending_hit = 0;
        }
        return 1;
    }
    BfmWoodsLardwickEvents effects = {0};
    BfmBinchoReveal stage = bfm_bincho_reveal_step(field, run, eye, &effects);
    events->sound = effects.sound;
    events->reveal = stage == BFM_BINCHO_REVEAL_SPAWN;
    events->captive_yaw = field->soldier_yaw;
    events->approach = stage == BFM_BINCHO_REVEAL_APPROACH;
    actor->uncovered = events->approach;
    return stage != BFM_BINCHO_REVEAL_INVALID;
}

int bfm_valley_bincho_draw(const BfmValleyBincho *actor, BfmBedroomFigure *figure,
                           const BfmBedroomView *view)
{
    if (!actor || !figure || !view) return 0;
    if (!actor->art || actor->uncovered || *actor->registered) return 1;
    const BfmWoodsLardwickField *field = &actor->crystal;
    if (field->crystal_visible && !bfm_woods_resident_glow_art_add(actor->art, figure, view,
        field->root.xyz, field->root.yaw, field->light_pitch, field->light_yaw, field->flash,
        field->lights_active, field->burst_updates, field->glow_visible)) return 0;
    return !field->glow_scale || bfm_woods_resident_release_glow_add(actor->art, figure, view,
        &field->glow_transform, field->glow_scale);
}

/* SPDX-License-Identifier: MIT */
#include "steam_knight_charge_effect.h"

#include <string.h>

static int floor_div(int64_t n, int divisor)
{
    return (int)(n / divisor - (n % divisor < 0));
}

static int16_t field_coord(int64_t value)
{
    uint16_t word = (uint16_t)value;
    return (int16_t)(word < 32768u ? (int)word : (int)word - 65536);
}

static int allocate_slot(BfmSteamChargeEffects *effects, uint8_t kind)
{
    for (int i = 0; i < BFM_STEAM_CHARGE_SLOTS; ++i) {
        if (!effects->slots[i].kind) {
            effects->slots[i] = (BfmSteamChargeSlot){.kind = kind};
            return i;
        }
    }
    return -1;
}

int bfm_steam_charge_reserve(BfmSteamChargeEffects *effects, uint8_t kind)
{
    if (!effects || !kind || kind == BFM_STEAM_CHARGE_EMITTER ||
        kind == BFM_STEAM_CHARGE_STREAK) return -1;
    return allocate_slot(effects, kind);
}

void bfm_steam_charge_release(BfmSteamChargeEffects *effects, unsigned index, uint8_t kind)
{
    if (effects && index < BFM_STEAM_CHARGE_SLOTS && kind &&
        kind != BFM_STEAM_CHARGE_EMITTER && kind != BFM_STEAM_CHARGE_STREAK &&
        effects->slots[index].kind == kind) effects->slots[index].kind = 0;
}

int bfm_steam_charge_enter(BfmSteamChargeEffects *effects, uint8_t equipped_scroll)
{
    if (!effects || (equipped_scroll & 0x80)) return 0;
    int index = allocate_slot(effects, BFM_STEAM_CHARGE_EMITTER);
    if (index < 0) return 0;
    effects->slots[index].sound_only = !equipped_scroll;
    return 1;
}

static void emitter_step(BfmSteamChargeEffects *effects, unsigned index, unsigned gauge,
                         const BfmForestTransform *model, const int8_t anchors[2][3])
{
    BfmSteamChargeSlot *slot = &effects->slots[index];
    if (!slot->started) {
        slot->started = 1;
        return;
    }
    if (!gauge) {
        effects->stop_charge = 1;
        slot->kind = 0;
        return;
    }
    if (gauge < 24) return;
    if (!slot->sound_only) {
        effects->centers |= UINT32_C(1) << index;
        int center[3];
        for (unsigned i = 0; i < 3; ++i)
            center[i] = floor_div(anchors[0][i] + anchors[1][i], 2);
        for (unsigned i = 0; i < 3; ++i) {
            int64_t dot = 0;
            for (unsigned j = 0; j < 3; ++j) dot += model->rotation[i][j] * center[j];
            slot->tip[i] = field_coord((int64_t)model->translation[i] + floor_div(dot, 4096));
        }
    }
    if (++slot->timer == 4) effects->play_charge = 1;
    if (gauge < 128) {
        if (slot->sound_only) return;
        int child = allocate_slot(effects, BFM_STEAM_CHARGE_STREAK);
        if (child >= 0) effects->slots[child].parent = (uint8_t)index;
    } else {
        if (!slot->full) {
            slot->full = 1;
            slot->timer = 256;
        }
        if (!(slot->timer & 31)) effects->play_full = 1;
    }
}

static void start_streak(BfmSteamChargeSlot *slot, const BfmSteamChargeSlot *parent,
                         const BfmForestTransform *model, unsigned roll)
{
    int pitch_angle = (int)((roll & 0x7f00u) >> 5) - 768;
    unsigned yaw_angle = ((roll & 127u) << 4) + 1024;
    const int16_t zero[3] = {0};
    BfmForestTransform pitch, yaw;
    bfm_forest_actor_root((unsigned)(pitch_angle < 0 ? -pitch_angle : pitch_angle), zero, &pitch);
    bfm_forest_actor_root(yaw_angle, zero, &yaw);
    int cos_pitch = pitch.rotation[0][0];
    int sin_pitch = pitch_angle < 0 ? -pitch.rotation[0][2] : pitch.rotation[0][2];
    /* Only the local Z axis contributes to the offset. Compose its rotation
     * with the owner's basis before scaling the resulting axis by the radius. */
    int axis[3] = {floor_div(yaw.rotation[0][2] * cos_pitch, 4096), -sin_pitch,
                   floor_div(yaw.rotation[0][0] * cos_pitch, 4096)};
    for (unsigned i = 0; i < 3; ++i) {
        int64_t dot = 0;
        for (unsigned j = 0; j < 3; ++j) dot += (int64_t)model->rotation[i][j] * axis[j];
        int delta = floor_div(-80 * floor_div(dot, 4096), 4096);
        slot->previous[i] = field_coord(parent->tip[i] + delta);
        slot->tip[i] = slot->previous[i];
    }
    slot->started = 1;
}

static void streak_step(BfmSteamChargeEffects *effects, unsigned index,
                        const BfmForestTransform *model, BfmGameplayRun *run)
{
    BfmSteamChargeSlot *slot = &effects->slots[index];
    const BfmSteamChargeSlot *parent = &effects->slots[slot->parent];
    if (!slot->started) {
        unsigned roll = bfm_gameplay_random(run);
        if (parent->kind == BFM_STEAM_CHARGE_EMITTER && model)
            start_streak(slot, parent, model, roll);
        else {
            /* A newborn still owns this visit and its draw. Leave it in the
             * pool until its next turn, with no geometry from a retired owner. */
            slot->started = 1;
            slot->age = 4;
        }
        return;
    }
    if (parent->kind != BFM_STEAM_CHARGE_EMITTER || slot->age >= 4) {
        slot->kind = 0;
        return;
    }
    BfmSteamChargeLine *line = &effects->lines[effects->line_count++];
    line->slot = index;
    for (unsigned i = 0; i < 3; ++i) {
        line->from[i] = field_coord(floor_div((int64_t)slot->previous[i] + slot->tip[i], 2));
        line->to[i] = field_coord(floor_div(field_coord(slot->tip[i]) + field_coord(parent->tip[i]), 2));
        slot->previous[i] = field_coord(slot->tip[i]);
        slot->tip[i] = line->to[i];
    }
    ++slot->age;
}

int bfm_steam_charge_step_shared(BfmSteamChargeEffects *effects, BfmGameplayRun *run,
                                 unsigned gauge, const BfmForestTransform *model,
                                 const int8_t anchors[2][3],
                                 BfmSteamChargeOtherEffect visit, void *context)
{
    if (!effects || !run || gauge > 128) return 0;
    if (!model || !anchors) {
        for (unsigned i = 0; i < BFM_STEAM_CHARGE_SLOTS; ++i) {
            const BfmSteamChargeSlot *slot = &effects->slots[i];
            if (slot->kind == BFM_STEAM_CHARGE_EMITTER && slot->started && !slot->sound_only && gauge >= 24) return 0;
            if (!model && slot->kind == BFM_STEAM_CHARGE_STREAK && !slot->started &&
                effects->slots[slot->parent].kind == BFM_STEAM_CHARGE_EMITTER) return 0;
        }
    }
    effects->line_count = 0;
    effects->centers = 0;
    effects->play_charge = effects->stop_charge = effects->play_full = 0;
    /* Higher newborn slots are reached in this pass; lower newborn slots
     * consume their first update, including their random draw, on the next. */
    for (unsigned i = 0; i < BFM_STEAM_CHARGE_SLOTS; ++i) {
        if (effects->slots[i].kind == BFM_STEAM_CHARGE_EMITTER)
            emitter_step(effects, i, gauge, model, anchors);
        else if (effects->slots[i].kind == BFM_STEAM_CHARGE_STREAK)
            streak_step(effects, i, model, run);
        else if (effects->slots[i].kind && visit)
            visit(context, i, effects->slots[i].kind);
    }
    return 1;
}

int bfm_steam_charge_step(BfmSteamChargeEffects *effects, BfmGameplayRun *run,
                          unsigned gauge, const BfmForestTransform *model,
                          const int8_t anchors[2][3])
{
    return bfm_steam_charge_step_shared(effects, run, gauge, model, anchors, NULL, NULL);
}

unsigned bfm_steam_charge_project(const BfmSteamChargeEffects *effects,
                                  const BfmForestCamera *camera,
                                  BfmForestDustCard *cards, unsigned capacity)
{
    if (!effects || !camera || !cards) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < effects->line_count && count < capacity; ++i) {
        const BfmSteamChargeLine *line = &effects->lines[i];
        int16_t corners[4][3];
        for (unsigned j = 0; j < 4; ++j)
            memcpy(corners[j], (j & 1) ? line->to : line->from, sizeof corners[j]);
        BfmForestDustCard card = {0};
        if (!bfm_forest_project_effect_quad(camera, corners, &card)) continue;
        card.vertices[0].rgb = 0x001000;
        card.vertices[1].rgb = 0x208020;
        card.material = (BfmForestRasterMaterial){.count = 2, .translucent = 1, .page = 1u << 5};
        card.slot = line->slot;
        cards[count++] = card;
    }
    return count;
}

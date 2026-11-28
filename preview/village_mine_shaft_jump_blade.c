/* SPDX-License-Identifier: MIT */
#include "village_mine_shaft_jump_blade.h"
#include "forest_field.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

static int load_jump_blade(BfmShaftBladeData *data, const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, size_t moves, size_t powers, size_t sounds,
    size_t box_profile)
{
    const size_t move_offset = moves + 6 * 20;
    if (!data || !scene || bytes < move_offset + 20 || bytes < powers + 8 * 12 ||
        bytes < sounds + 16 || (box_profile && bytes < box_profile + 2 * 52)) return 0;
    const uint8_t *move = scene + move_offset;
    unsigned event = u32le(move);
    if (event != 7 || u32le(move + 4) != 0 || move[8] != 11 ||
        u32le(move + 12) >= 4 || u16le(move + 18) != 1) return 0;
    const size_t power_offset = powers + 12 * event;
    const uint8_t *power = scene + power_offset;
    unsigned mode = u16le(power);
    if ((mode & 0xf000) != 0x4000 || u16le(power + 10) != 0) return 0;
    BfmShaftBladeData loaded = {
        .power = {mode & 0xfff, u16le(power+2), u16le(power+4), u16le(power+6), 0},
        .event = event, .effect = move[8], .variant = move[9]
    };
    if (!bfm_weapon_ranks_load(disc, loaded.ranks)) return 0;
    for (unsigned i = 0; i < 8; ++i) loaded.surface_sound[i] = u16le(scene + sounds + 2*i);
    for (unsigned i = 0; box_profile && i < 2; ++i) {
        const uint8_t *profile = scene + box_profile + 52*i;
        if (u16le(profile+2) != 0x8c00) return 0;
        loaded.box_sound[i] = u16le(profile + 0x14 + 2*u32le(move+12));
    }
    *data = loaded;
    return 1;
}

int bfm_shaft_jump_blade_load(BfmShaftBladeData *data, const MusashiDiscMedia *disc,
                              const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    return load_jump_blade(data, disc, scene, bytes, 0x8018020c - base, 0x801803b0 - base,
                           0x80180620 - base, 0x80197174 - base);
}

int bfm_lake_jump_blade_load(BfmShaftBladeData *data, const MusashiDiscMedia *disc,
                             const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t binding = 0x80181d20 - base;
    if (!scene || bytes < binding + 4 || u32le(scene + binding) != UINT32_C(0x80185a50)) return 0;
    return load_jump_blade(data, disc, scene, bytes, 0x80183c2c - base, 0x80183dd0 - base,
                           0x80184040 - base, 0);
}

void bfm_shaft_jump_blade_cancel(BfmShaftJumpBlade *blade)
{
    if (blade) blade->interrupted = (BfmBedroomBodyCursor){0};
}

int bfm_shaft_jump_blade_active(const BfmShaftJumpBlade *blade)
{
    return blade && blade->interrupted.program;
}

int bfm_shaft_jump_blade_step(BfmShaftJumpBlade *blade, const BfmShaftBladeData *data,
    BfmBedroomMotion *motion, BfmGameplayRun *run, const BfmForestTransform *model,
    int square_pressed, uint32_t actor_flags, BfmBedroomPose *body, BfmShaftBladeFrame *frame)
{
    if (!blade || !data || !motion || !body || !frame) return 0;
    *frame = (BfmShaftBladeFrame){0};
    if (bfm_shaft_jump_blade_active(blade)) {
        if (bfm_bedroom_motion_marker(motion) == BFM_BODY_END) {
            if (!bfm_bedroom_motion_ability_pose(motion, 34, 0, body) ||
                !bfm_bedroom_motion_restore_body(motion, blade->interrupted)) return 0;
            bfm_shaft_jump_blade_cancel(blade);
            bfm_bedroom_motion_publish_upright(motion, body);
            return 1;
        }
    } else {
        if (!square_pressed || (actor_flags & 0x404)) return 1;
        BfmBedroomBodyCursor prior = bfm_bedroom_motion_body_cursor(motion);
        unsigned slot;
        if (prior.program == UINT32_C(0x800d4a9c)) slot = 11;
        else if (prior.program == UINT32_C(0x800d4b48)) slot = 12;
        else return 1;
        if (!run || !run->has_player || !model) return 0;
        if (!bfm_bedroom_motion_strike_slot(motion, slot) ||
            !bfm_bedroom_motion_ability_pose(motion, 34, 0, body)) return 0;
        blade->interrupted = prior;
        blade->hard_sound_played = 0;
        frame->started = 1;
        bfm_bedroom_motion_publish_upright(motion, body);
    }

    const uint8_t *next_pose;
    if (!bfm_bedroom_motion_strike_next(motion, &next_pose)) return 0;
    if (next_pose) {
        if (!model || !body->pose) return 0;
        frame->rays = (unsigned)bfm_forest_blade_rays(model, (const int8_t *)body->pose,
                            (const int8_t *)next_pose, frame->from, frame->to);
    }
    BfmSteamSpinRoll roll;
    if (!bfm_weapon_roll(data->ranks, &data->power, run, actor_flags, &roll)) return 0;
    frame->event = data->event;
    frame->strength = roll.strength;
    return 1;
}

unsigned bfm_shaft_jump_blade_contacts(BfmShaftJumpBlade *blade, const BfmShaftBladeData *data,
    const BfmVillageGround *ground, const BfmWoods3026BoxHit *box,
    const BfmShaftBladeFrame *frame, BfmShaftBladeImpact impacts[3])
{
    if (!blade || !data || !ground || !frame || !impacts || frame->rays > 3) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < frame->rays; ++i) {
        BfmShaftBladeImpact hit = {0};
        memcpy(hit.point, frame->to[i], sizeof hit.point);
        uint16_t material = 0;
        int terrain = bfm_village_ground_blade(ground, frame->from, hit.point, hit.normal, &material);
        if (box) {
            int16_t center[3];
            if (!(blade->box_contacts & 1) && bfm_forest_guard_contact_point(&box->bounds,
                    &box->root, frame->from, hit.point, center, hit.point, hit.normal)) {
                hit.box_part = 1;
                blade->box_contacts |= 1;
            }
            if (!(blade->box_contacts & 2) && bfm_forest_guard_contact_point(&box->lid_bounds,
                    &box->lid_root, frame->from, hit.point, center, hit.point, hit.normal)) {
                hit.box_part = 2;
                blade->box_contacts |= 2;
            }
        }
        if (hit.box_part) {
            hit.type = data->effect;
            hit.variant = data->variant;
            hit.sound = data->box_sound[hit.box_part-1];
        } else if (terrain) {
            hit.type = 6;
            if (!blade->hard_sound_played) hit.sound = data->surface_sound[material>>13];
            blade->hard_sound_played = 1;
        } else {
            continue;
        }
        impacts[count++] = hit;
    }
    return count;
}

/* SPDX-License-Identifier: MIT */
#include "woods_player_hit.h"
#include "forest_hurt.h"
#include "woods_caster_ring.h"
#include "forest_pose.h"
#include <stdlib.h>

struct BfmWoodsPlayerHit {
    const MusashiDiscMedia *disc;
    BfmForestHurtClip clip;
    BfmForestHurt hurt;
    BfmForestDamageDigits digits;
    uint16_t damage;
    uint8_t palette;
    unsigned blink, recoil;
    int airborne, guard_closed;
    int16_t local_axis[2];
};

BfmWoodsPlayerHit *bfm_woods_player_hit_open(const MusashiDiscMedia *disc,
                                  const uint8_t *scene, size_t bytes)
{
    const size_t hurt_palette = UINT32_C(0x8018a975) - UINT32_C(0x80128158);
    if (!disc || !scene || hurt_palette >= bytes) return NULL;
    BfmWoodsPlayerHit *hit = calloc(1, sizeof *hit);
    if (!hit) return NULL;
    if (!bfm_forest_hurt_load(disc, &hit->clip)) {
        free(hit);
        return NULL;
    }
    hit->disc = disc;
    hit->palette = scene[hurt_palette];
    return hit;
}

void bfm_woods_player_hit_free(BfmWoodsPlayerHit *hit)
{
    free(hit);
}

static int faces_attack(unsigned player, unsigned attacker)
{
    unsigned delta = (attacker - (player + 2048)) & 4095u;
    return (delta < 2048 ? delta : 4095 - delta) < 682;
}

static int32_t recoil_world(const int16_t row[3], const int16_t local[2], int half_units)
{
    /* Q12 heading, Q16 half-unit steps, rotated back from the hero basis. */
    int64_t dot = (int64_t)row[0] * local[0] + (int64_t)row[2] * local[1];
    int64_t scaled = -dot * half_units * 32768;
    const int resolution = 4096 * 4096;
    return (int32_t)(scaled / resolution - (scaled % resolution < 0));
}

static void step_guard(BfmWoodsPlayerHit *hit, const BfmGameplayRun *run,
                       const BfmWoodsPlayerHitInput *input, BfmWoodsPlayerHitFrame *frame)
{
    const BfmBedroomRoot *root = input->root;
    frame->movement_locked = 1;
    if (input->square_pressed || input->triangle_pressed) {
        BfmForestStatusValues status = bfm_forest_status_values(&run->player);
        if ((status.orb & 127) && status.charge == 128)
            frame->guard_cancel = BFM_WOODS_HIT_CHARGED_ATTACK;
        else if ((status.orb & 127) && input->triangle_pressed && bfm_gameplay_story_flag(run, 128) &&
                 !frame->miniature && !(input->player_flags & 0x400))
            frame->guard_cancel = BFM_WOODS_HIT_LUMINA_ATTACK;
        else if (input->square_pressed && !frame->miniature && !(input->player_flags & 0x404))
            frame->guard_cancel = BFM_WOODS_HIT_FUSION_ATTACK;
    }
    if (frame->guard_cancel) frame->cancel_countdown = hit->recoil;
    if (frame->guard_cancel || !--hit->recoil) {
        hit->recoil = 0;
        hit->guard_closed = 1;
        frame->recovered = 1;
        return;
    }
    BfmForestTransform world;
    bfm_forest_actor_root(root->yaw, root->xyz, &world);
    int half_units = (int)hit->recoil + 4;
    frame->recoil_x = recoil_world(world.rotation[0], hit->local_axis, half_units);
    frame->recoil_z = recoil_world(world.rotation[2], hit->local_axis, half_units);
}

void bfm_woods_player_hit_guard_contact(BfmWoodsPlayerHit *hit, BfmWoodsPlayerHitContact contact,
                                  BfmWoodsPlayerHitFrame *frame)
{
    if (!hit || !frame || !hit->recoil) return;
    if (contact == BFM_WOODS_HIT_LOST_SUPPORT || contact == BFM_WOODS_HIT_SURFACE_REACTION ||
        contact == BFM_WOODS_HIT_SOLID_ACTOR_REACTION) {
        frame->resume_fall = contact == BFM_WOODS_HIT_LOST_SUPPORT;
        frame->surface_reaction = contact == BFM_WOODS_HIT_SURFACE_REACTION;
        frame->solid_actor_reaction = contact == BFM_WOODS_HIT_SOLID_ACTOR_REACTION;
        frame->recovered = 1;
        hit->guard_closed = 1;
        hit->recoil = 0;
    }
}

static void finish_hurt(BfmWoodsPlayerHit *hit, BfmGameplayRun *run, uint32_t player_flags,
                        BfmWoodsPlayerHitContact contact, BfmWoodsPlayerHitFrame *frame)
{
    unsigned damage = hit->damage;
    if (player_flags & UINT32_C(0x02000000))
        damage = bfm_forest_status_values(&run->player).max_hp;
    bfm_forest_status_debit(&run->player, damage);
    hit->hurt.active = 0;
    hit->blink = 32;
    frame->movement_locked = 1;
    frame->pose = 0;
    frame->zero_hp = bfm_forest_status_values(&run->player).hp <= 0;
    if (!frame->zero_hp) {
        frame->recovered = 1;
        frame->surface_reaction = contact == BFM_WOODS_HIT_SURFACE_REACTION;
        frame->resume_fall = contact == BFM_WOODS_HIT_LOST_SUPPORT ||
            (hit->airborne && contact != BFM_WOODS_HIT_LANDED && !frame->surface_reaction);
    }
}

void bfm_woods_player_hit_hurt_contact(BfmWoodsPlayerHit *hit, BfmGameplayRun *run, uint32_t player_flags,
                                 BfmWoodsPlayerHitContact contact, BfmWoodsPlayerHitFrame *frame)
{
    if (!hit || !run || !frame || !hit->hurt.active) return;
    if ((hit->airborne && (contact == BFM_WOODS_HIT_LANDED || contact == BFM_WOODS_HIT_SURFACE_REACTION)) ||
        (!hit->airborne && contact == BFM_WOODS_HIT_LOST_SUPPORT))
        finish_hurt(hit, run, player_flags, contact, frame);
}

int bfm_woods_player_hit_step(BfmWoodsPlayerHit *hit, BfmGameplayRun *run,
                        const BfmWoodsPlayerHitInput *input, BfmWoodsPlayerHitFrame *frame)
{
    if (!hit || !run || !input || !input->root || !frame) return 0;
    *frame = (BfmWoodsPlayerHitFrame){0};
    if (!(input->protected_action & UINT32_C(0x80ffffff)) &&
        input->player_action != 26 && input->player_action != 30)
        bfm_woods_ring_mini_step(&run->player);
    frame->miniature = bfm_woods_ring_mini_active(&run->player);
    int ignore_hit = hit->blink != 0 || hit->guard_closed;
    hit->guard_closed = 0;
    if (hit->blink) frame->hide_body = (--hit->blink & 1u) != 0;
    bfm_forest_damage_digits_advance(&hit->digits);
    const BfmBedroomRoot *root = input->root;
    if (!ignore_hit && !hit->hurt.active && !hit->recoil &&
        (input->hit_flags & 0xffu) == 1 &&
        input->receives_damage && bfm_forest_status_values(&run->player).hp > 0) {
        if (!(input->hit_flags & 0x4000) && input->guard_ready && faces_attack(root->yaw, input->attacker_yaw)) {
            BfmForestTransform relative;
            bfm_forest_actor_root(input->attacker_yaw - root->yaw, root->xyz, &relative);
            hit->local_axis[0] = relative.rotation[0][2];
            hit->local_axis[1] = relative.rotation[2][2];
            hit->recoil = 8;
            frame->guard_started = 1;
            frame->sound = 1085;
        } else if (input->player_action != 25 && input->player_action != 26 &&
                   (!input->protected_action || (input->protected_action & UINT32_C(0x02000000)))) {
            uint16_t damage = 0;
            if (input->power && !bfm_forest_hit_damage(hit->disc, &run->player,
                    input->player_flags, input->power, &damage)) return 0;
            if (!bfm_forest_hurt_begin(&hit->hurt, root->yaw, input->attacker_yaw)) return 0;
            hit->damage = damage;
            hit->airborne = input->airborne;
            frame->hurt_started = 1;
            frame->release_carry = input->carrying;
            frame->sound = 1105;
            if (damage) {
                bfm_forest_damage_digits_start_data(&hit->digits, &hit->palette, root->xyz, damage);
                hit->digits.hurt = 1;
                bfm_forest_damage_digits_scatter(&hit->digits, bfm_gameplay_random(run));
            }
            if ((input->hit_flags & 0x1000) && !frame->miniature) {
                bfm_woods_ring_mini_apply(&run->player);
                frame->applied_mini = 1;
            }
        }
    }
    if (hit->digits.amount) frame->digits = &hit->digits;
    if (hit->recoil) {
        if (frame->guard_started && (input->contact == BFM_WOODS_HIT_LOST_SUPPORT ||
                                     input->contact == BFM_WOODS_HIT_SURFACE_REACTION)) {
            frame->guard_started = 0;
            frame->sound = 0;
            frame->movement_locked = 1;
            bfm_woods_player_hit_guard_contact(hit, input->contact, frame);
        } else {
            step_guard(hit, run, input, frame);
        }
    } else if (hit->hurt.active) {
        frame->movement_locked = 1;
        if (!frame->hurt_started)
            bfm_woods_player_hit_hurt_contact(hit, run, input->player_flags, input->contact, frame);
        if (hit->hurt.active) {
            BfmForestHurtResult result = bfm_forest_hurt_step(&hit->hurt, &hit->clip,
                                                              NULL, NULL, &frame->pose);
            if (result == BFM_FOREST_HURT_RECOVERED)
                finish_hurt(hit, run, input->player_flags, input->contact, frame);
            else if (result != BFM_FOREST_HURT_LOCKED) return 0;
        }
    }
    return 1;
}

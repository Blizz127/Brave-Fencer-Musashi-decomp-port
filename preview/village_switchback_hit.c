/* SPDX-License-Identifier: MIT */
#include "village_switchback_hit.h"
#include "forest_cast.h"
#include "field_waypoint.h"
#include "village_field.h"
#include "woods_fusion_art.h"
#include "woods_fusion.h"
#include "woods_player_hit.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

struct BfmSwitchbackHit {
    BfmWoodsPlayerHit *receiver;
    BfmWoodsPlayerHitFrame frame;
    BfmForestDamageDigits credit;
    BfmForestCast cast;
    uint8_t credit_palette;
    int guarding, tracking_fall;
    unsigned camera_offset;
    int16_t last[3], previous[3], hurt_target[3];
};

static BfmSwitchbackHit *open_hit(const MusashiDiscMedia *disc, const uint8_t *scene, size_t bytes,
    const int16_t feet[3], size_t room, size_t palettes)
{
    if (!disc || !scene || !feet || bytes < room + 0x50 || bytes < palettes + 6) return NULL;
    BfmSwitchbackHit *hit = calloc(1, sizeof *hit);
    if (!hit) return NULL;
    hit->receiver = bfm_woods_player_hit_open_palette(disc, scene[palettes + 1]);
    if (!hit->receiver || !bfm_forest_cast_load(&hit->cast, disc, u32le(scene + room + 0x4c))) {
        bfm_switchback_hit_free(hit);
        return NULL;
    }
    bfm_woods_player_hit_enable_bp_drain(hit->receiver, scene[palettes + 5]);
    hit->credit_palette = scene[palettes + 4];
    memcpy(hit->last, feet, sizeof hit->last);
    memcpy(hit->previous, feet, sizeof hit->previous);
    return hit;
}

BfmSwitchbackHit *bfm_switchback_hit_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, const int16_t feet[3])
{
    return open_hit(disc, scene, bytes, feet, 0x80184870u - 0x80128158u, 0x80183e04u - 0x80128158u);
}

BfmSwitchbackHit *bfm_switchback_hit_open_lake(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, const int16_t feet[3])
{
    return open_hit(disc, scene, bytes, feet, 0x80185a50u - 0x80128158u, 0x80184ed0u - 0x80128158u);
}

void bfm_switchback_hit_free(BfmSwitchbackHit *hit)
{
    if (!hit) return;
    bfm_woods_player_hit_free(hit->receiver);
    free(hit);
}

/* The US pad normalizes each stick axis before the stance checks its heading. */
static int stance_axis(int value)
{
    if (value >= -47 && value <= 47) return 0;
    if (value <= -121) return -128;
    if (value >= 120) return 127;
    return value;
}

int bfm_switchback_hit_step(BfmSwitchbackHit *hit, BfmVillageMotion *motion, BfmGameplayRun *run,
    BfmSaveRecord *save, BfmFieldInput input, unsigned camera_yaw, const BfmMineLungeHit *incoming)
{
    if (!hit || !run || !run->has_player || !save || !incoming) return -1;
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    if (!p) return -1;
    memcpy(run->player.record, save->progress, sizeof save->progress);
    uint8_t *record = run->player.record;
    int active = p->controlled && !bfm_village_motion_transfer(motion);
    int stance = hit->cast.action == BFM_CAST_STANCE && !(record[0x48] & 128u);
    BfmWoodsPlayerHitInput in = {
        .root=&p->root, .airborne=!bfm_village_motion_grounded(motion),
        .player_action=hit->frame.pose ? 25u : stance ? 17u : 0u,
        .player_flags=record[0x23] & 128u ? 0x400u : 0u,
        .receives_damage=active, .protected_action=active ? 0u : 1u,
        .guard_ready=stance, .hit_flags=incoming->flags,
        .power=incoming->power, .attacker_yaw=incoming->yaw,
        .square_pressed=input.square_pressed, .triangle_pressed=input.triangle_pressed
    };
    if (active && !(record[0x48] & 128u))
        bfm_woods_fusion_sample_charge(run, stance || hit->guarding, input.shoulder_held);
    BfmWoodsPlayerHitFrame *f = &hit->frame;
    if (!bfm_woods_player_hit_step(hit->receiver, run, &in, f)) return -1;
    bfm_forest_damage_digits_advance(&hit->credit);
    if (f->hurt_started) {
        memcpy(hit->hurt_target, hit->previous, sizeof hit->hurt_target);
        if (stance) record[0x47] = 0;
        hit->cast.action = BFM_CAST_GROUND;
        if (record[0x49] == 20) {
            BfmForestStatusValues s = bfm_forest_status_values(&run->player);
            unsigned raised = (unsigned)s.bp + 10;
            if (raised > s.max_bp) raised = s.max_bp;
            record[0x40] = (uint8_t)raised;
            record[0x41] = (uint8_t)(raised >> 8);
            bfm_forest_damage_digits_start_data(&hit->credit, &hit->credit_palette, p->root.xyz, 10);
            hit->credit.bp_credit = 1;
        }
    }
    if (f->guard_started) hit->guarding = 1;
    if (f->movement_locked) {
        BfmWoodsMotionContact ground;
        BfmWoodsReactionMotion kind = hit->guarding ? BFM_WOODS_GUARD_RECOIL :
                                      f->hurt_started ? BFM_WOODS_HURT_START : BFM_WOODS_HURT_HOLD;
        if (!bfm_village_motion_woods_reaction(motion, NULL, kind, f->recoil_x, f->recoil_z, &ground))
            return -1;
        BfmWoodsPlayerHitContact contact = BFM_WOODS_HIT_SAME_SUPPORT;
        if (ground.lost_support) contact = BFM_WOODS_HIT_LOST_SUPPORT;
        else if (ground.slope & 0x4000) contact = BFM_WOODS_HIT_SURFACE_REACTION;
        else if (ground.landed) contact = BFM_WOODS_HIT_LANDED;
        if (hit->guarding) bfm_woods_player_hit_guard_contact(hit->receiver, contact, f);
        else bfm_woods_player_hit_hurt_contact(hit->receiver, run, in.player_flags, contact, f);
        if (f->pose && !bfm_village_motion_combat_face(motion, 22)) return -1;
        if (f->recovered) {
            hit->guarding = 0;
            if (!bfm_village_motion_woods_finish_hurt(motion, bfm_field_tired_stance(record))) return -1;
        }
    }
    int consumed = f->movement_locked;
    if (!consumed && active) {
        int held = input.shoulder_held && !input.square_pressed && !input.triangle_pressed &&
                   !(record[0x48] & 128u) && !f->miniature && bfm_village_motion_can_examine(motion);
        /* A learned evasion belongs to the action dispatcher. Route that
         * fresh Cross to ordinary input until the special action is mounted. */
        int evade = stance && input.cross_pressed && bfm_gameplay_story_flag(run, 30);
        if (held && !evade) {
            int x = stance_axis(input.x), y = stance_axis(input.y);
            int moving = stance && (x || y);
            unsigned stride_yaw = camera_yaw + bfm_waypoint_heading(-x, y);
            if (!stance) {
                hit->camera_offset = p->root.yaw - camera_yaw;
                hit->cast.visible = bfm_village_field_visible(&p->pose, save->flags);
            }
            int supported;
            if (!bfm_village_motion_switchback_stance(motion,
                    (camera_yaw + hit->camera_offset) & 4095u, stride_yaw, moving,
                    bfm_field_tired_stance(record), &supported)) return -1;
            bfm_forest_cast_after_charge(&hit->cast, &run->player, supported, 0, moving, 0);
            consumed = 1;
        } else if (hit->cast.action == BFM_CAST_STANCE) {
            bfm_forest_cast_after_charge(&hit->cast, &run->player, 0, 0, 0, 0);
            if (!input.cross_pressed && !input.square_pressed && !input.triangle_pressed) {
                int supported;
                if (!bfm_village_motion_switchback_stance(motion, p->root.yaw, 0, 0,
                        bfm_field_tired_stance(record), &supported)) return -1;
                if (supported && !bfm_village_motion_woods_finish_hurt(motion,
                        bfm_field_tired_stance(record))) return -1;
                consumed = 1;
            }
        }
    } else if (f->recovered && (!input.shoulder_held || f->guard_cancel ||
                                f->resume_fall || f->surface_reaction || f->solid_actor_reaction)) {
        bfm_forest_cast_after_charge(&hit->cast, &run->player, 0, 0, 0, 0);
    }
    memcpy(save->progress, record, sizeof save->progress);
    return consumed;
}

int bfm_switchback_hit_can_fall(const BfmSwitchbackHit *hit)
{
    return hit && (hit->guarding || !hit->frame.pose);
}

int bfm_switchback_hit_environment(BfmSwitchbackHit *hit, BfmGameplayRun *run,
    const BfmBedroomRoot *root, const BfmForestBankThornFrame *floor)
{
    BfmWoodsPlayerHitInput inactive = {.root = root, .player_action = 25};
    if (!hit || !run || !root || !floor) return 0;
    if (floor->started) {
        memcpy(hit->hurt_target, hit->previous, sizeof hit->hurt_target);
        hit->cast.action = BFM_CAST_GROUND;
        hit->guarding = 0;
        hit->tracking_fall = 1;
        bfm_woods_player_hit_blink(hit->receiver, 0);
    }
    if (!bfm_woods_player_hit_step(hit->receiver, run, &inactive, &hit->frame)) return 0;
    bfm_forest_damage_digits_advance(&hit->credit);
    if (floor->bp_credit) {
        bfm_forest_damage_digits_start_data(&hit->credit, &hit->credit_palette, floor->effect_root,
                                            floor->bp_credit);
        hit->credit.bp_credit = 1;
    }
    if (floor->recovered) hit->tracking_fall = 0;
    if (floor->blink_fields) bfm_woods_player_hit_blink(hit->receiver, floor->blink_fields);
    return 1;
}

const int16_t *bfm_switchback_hit_target(BfmSwitchbackHit *hit, BfmVillageMotion *motion)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    memcpy(hit->previous, hit->last, sizeof hit->previous);
    memcpy(hit->last, p->root.xyz, sizeof hit->last);
    return hit->tracking_fall || hit->frame.pose ? hit->hurt_target : p->root.xyz;
}

int bfm_switchback_hit_body(const BfmSwitchbackHit *hit, const uint8_t *main,
    size_t bytes, BfmBedroomPose *pose, unsigned *visible)
{
    uint32_t key = hit->frame.pose;
    if (hit->cast.action == BFM_CAST_STANCE) key = hit->cast.pose;
    if (key && !bfm_woods_fusion_art_pose(main, bytes, key, *visible, pose->hide_fusion, pose)) return 0;
    if (hit->frame.hide_body) *visible = 0;
    return 1;
}

int bfm_switchback_hit_draw(const BfmSwitchbackHit *hit, BfmBedroomFigure *figure,
    const BfmBedroomView *view)
{
    return bfm_woods_fusion_art_append(figure, view, NULL, NULL, hit->frame.digits) &&
           bfm_woods_fusion_art_append(figure, view, NULL, NULL, &hit->credit);
}

unsigned bfm_switchback_hit_take_sound(BfmSwitchbackHit *hit)
{
    if (!hit) return 0;
    unsigned sound = hit->frame.sound;
    hit->frame.sound = 0;
    return sound;
}

/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_blade.h"
#include "field_waypoint.h"
#include "woods_twinpeak_player_blade.h"
#include <stdlib.h>
#include <string.h>

struct BfmMineLakeBlade {
    BfmWoodsCasterMelee *melee;
    BfmWoodsCasterMeleeFrame frame;
};

BfmMineLakeBlade *bfm_mine_lake_blade_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes)
{
    BfmMineLakeBlade *blade = calloc(1, sizeof *blade);
    if (blade) {
        blade->melee = bfm_mine_lake_melee_open(disc, scene, bytes, main, main_bytes);
        if (!blade->melee) {
            free(blade);
            return NULL;
        }
    }
    return blade;
}

void bfm_mine_lake_blade_free(BfmMineLakeBlade *blade)
{
    if (blade) {
        bfm_woods_caster_melee_free(blade->melee);
        free(blade);
    }
}

int bfm_mine_lake_blade_player(BfmMineLakeBlade *blade, BfmVillageMotion *motion,
    BfmGameplayRun *run, const BfmSaveRecord *save, BfmFieldInput input, int pad_yaw, int claimed)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    if (!blade || !p || !save) return 0;
    if (run && run->has_player) {
        unsigned slot = claimed ? 0 : bfm_village_motion_mine_blade_slot(motion);
        int continuing = blade->frame.action == 5 || blade->frame.action == 6;
        BfmChargeDirection stick = bfm_field_charge_direction(input);
        int moving = stick.x || stick.y;
        BfmWoodsCasterMeleeInput attack = {
            /* At rest Cross precedes a new attack; the walking action checks
             * its two swords first. A running swing checks its own cancel. */
            .permitted = slot && (continuing || slot != 1 || !input.cross_pressed),
            .square_edge = input.square_pressed, .triangle_edge = input.triangle_pressed,
            .moving = moving && !input.shoulder_held, .jump_edge = input.cross_pressed,
            .actor_flags = (save->progress[0x23] & 0x80 ? 0x400u : 0) |
                           (p->pose.hide_fusion ? 4u : 0)
        };
        if (!bfm_mine_lake_melee_step(blade->melee, run, &p->root, &attack, &blade->frame))
            return 0;
        if (blade->frame.action) {
            int yaw = continuing && blade->frame.sound && moving ?
                (pad_yaw + bfm_waypoint_heading(-stick.x, stick.y)) & 4095 : -1;
            return bfm_village_motion_mine_blade_hold(motion, 0, yaw);
        }
        if (continuing && slot && !moving && !input.cross_pressed)
            return bfm_village_motion_mine_blade_hold(motion, 1, -1);
    } else {
        blade->frame = (BfmWoodsCasterMeleeFrame){0};
    }
    if (claimed) return 1;
    if (!bfm_village_motion_step_mine_lake_field(motion, input, pad_yaw, save, run))
        return 0;

    /* The airborne strike owns the jump pose; its physical segments go to the
     * same room receivers as the grounded Fusion and Lumina segments. */
    const BfmShaftBladeFrame *air = bfm_village_motion_mine_shaft_blade(motion);
    if (air) {
        blade->frame.event = air->event;
        blade->frame.strength = air->strength;
        blade->frame.sound = air->started ? 0x400 : 0;
        blade->frame.rays = air->rays;
        memcpy(blade->frame.from, air->from, sizeof air->from);
        memcpy(blade->frame.to, air->to, sizeof air->to);
    }
    return 1;
}

const BfmWoodsCasterMeleeFrame *bfm_mine_lake_blade_frame(const BfmMineLakeBlade *blade)
{
    return blade ? &blade->frame : NULL;
}

int bfm_mine_lake_blade_body(const BfmMineLakeBlade *blade, const uint8_t *main,
                             size_t main_bytes, BfmBedroomPose *pose)
{
    return blade && bfm_twinpeak_player_blade_pose(&blade->frame, main, main_bytes, pose);
}

void bfm_mine_lake_blade_connected(BfmMineLakeBlade *blade)
{
    if (blade) bfm_woods_caster_melee_connected(blade->melee);
}

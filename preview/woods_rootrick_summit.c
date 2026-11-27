/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_summit.h"
#include "woods_rootrick_finale_player.h"
#include "woods_caster_ring.h"
#include "forest_pose.h"
#include <stdlib.h>

struct BfmWoodsRootrickSummit {
    const uint8_t *scene;
    size_t bytes;
    BfmVillageMotion *motion;
    BfmWoodsRootrickActor *rival;
    BfmRootrickFinalePlayer *player;
    BfmWoodsRootrickBoulder *rock;
    BfmWoodsRootrickFinale actor;
    BfmForestGuardBox target[2];
    uint32_t grants;
};

BfmWoodsRootrickSummit *bfm_woods_rootrick_summit_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    BfmVillageMotion *motion, BfmWoodsRootrickActor *rival, int visible, unsigned outcome)
{
    if (!disc || !scene || !main || !motion || !rival) return NULL;
    BfmWoodsRootrickSummit *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->scene = scene;
    s->bytes = bytes;
    s->motion = motion;
    s->rival = rival;
    s->player = bfm_woods_rootrick_player_open(disc, scene, bytes, main, main_bytes, motion);
    if (!s->player || !bfm_woods_rootrick_boulder_target_boxes(scene, bytes, s->target) ||
        !bfm_woods_rootrick_finale_begin(&s->actor, rival, outcome, visible, scene, bytes) ||
        !bfm_woods_rootrick_player_begin(s->player, outcome)) {
        bfm_woods_rootrick_summit_free(s);
        return NULL;
    }
    return s;
}

void bfm_woods_rootrick_summit_free(BfmWoodsRootrickSummit *s)
{
    if (!s) return;
    bfm_woods_rootrick_boulder_free(s->rock);
    bfm_woods_rootrick_player_free(s->player);
    free(s);
}

int bfm_woods_rootrick_summit_draw(const BfmWoodsRootrickSummit *s, BfmBedroomFigure *figure,
                                   const BfmBedroomView *view)
{
    return !s || !s->rock || bfm_woods_rootrick_boulder_add(s->rock, figure, view);
}

static int spawn_boulder(BfmWoodsRootrickSummit *s, unsigned variant)
{
    if (s->rock || variant > 1 || s->actor.outcome != variant + 1) return 0;
    BfmWoodsRootrickActor *rival = s->rival;
    int32_t hero[3];
    if (!bfm_village_motion_position_q16(s->motion, hero)) return 0;
    uint16_t fraction[3];
    for (unsigned axis = 0; axis < 3; ++axis) fraction[axis] = (uint16_t)hero[axis];
    const int16_t *spot = rival->xyz;
    if (variant == BFM_BOULDER_MUSASHI_THROWS) {
        int32_t x = (int32_t)rival->xyz[0] * 65536 + rival->fraction[0];
        if (!bfm_woods_rootrick_player_align_x(s->player, x)) return 0;
        spot = bfm_village_motion_player(s->motion)->root.xyz;
        fraction[0] = rival->fraction[0];
    } else {
        rival->xyz[0] = bfm_village_motion_player(s->motion)->root.xyz[0];
        rival->fraction[0] = fraction[0];
    }
    s->rock = bfm_woods_rootrick_boulder_spawn(s->scene, s->bytes,
        bfm_village_motion_ground(s->motion), (BfmBoulderThrower)variant, spot, fraction);
    return s->rock != NULL;
}

int bfm_woods_rootrick_summit_command(BfmWoodsRootrickSummit *s, const BfmRootrickCommand *cmd)
{
    if (!s || !cmd) return -1;
    uint32_t value = cmd->operand;
    switch (cmd->event) {
    case BFM_ROOTRICK_TURN_ACTOR:
        bfm_woods_rootrick_finale_turn(&s->actor, s->rival, (int16_t)value);
        return 1;
    case BFM_ROOTRICK_DRIVE_ACTOR:
        bfm_woods_rootrick_finale_drive(&s->actor, s->rival, (int16_t)value, value >> 16);
        return 1;
    case BFM_ROOTRICK_ACTOR_VISIBLE:
        s->actor.visible = value != 0;
        return 1;
    case BFM_ROOTRICK_SUMMIT_BOULDER:
        return spawn_boulder(s, value) ? 1 : -1;
    case BFM_ROOTRICK_ACTOR_STAGE:
        return bfm_woods_rootrick_finale_stage(&s->actor, value) ? 1 : -1;
    default: {
        BfmForestTransform rival;
        bfm_forest_actor_pitch_yaw((unsigned)s->rival->pitch, s->rival->yaw, s->rival->xyz, &rival);
        return bfm_woods_rootrick_player_queue(s->player, cmd, &rival);
    }
    }
}

int bfm_woods_rootrick_summit_step(BfmWoodsRootrickSummit *s, BfmGameplayRun *run,
    int camera_yaw, int standing, BfmWoodsSummitUpdate *out)
{
    if (!s || !run || !out) return 0;
    *out = (BfmWoodsSummitUpdate){0};
    const BfmVillagePlayer *hero = bfm_village_motion_player(s->motion);
    BfmForestTransform hero_model, rival_model;
    if (!bfm_bedroom_figure_actor_root(&hero->root, hero->pose.pitch, &hero_model)) return 0;
    bfm_forest_actor_pitch_yaw((unsigned)s->rival->pitch, s->rival->yaw, s->rival->xyz, &rival_model);
    BfmRootrickPlayerFrame frame;
    unsigned flags = bfm_woods_ring_mini_active(&run->player) ? 0x400u : 0;
    if (!bfm_woods_rootrick_player_step(s->player, run, camera_yaw, flags,
                                       s->actor.outcome == 2 && s->rock, &frame)) return 0;
    s->grants = frame.granted_control_bits;
    if (frame.carry.attached || frame.carry.event == BFM_CARRY_THROW) {
        if (!bfm_woods_rootrick_boulder_carry(s->rock, frame.carry.socket,
                                             frame.socket_turn ? (int)frame.socket_yaw : -1,
                                             frame.carry.event == BFM_CARRY_THROW)) return 0;
    }
    int16_t contact[3], normal[3];
    int hit = bfm_woods_rootrick_player_rival_hit(s->player, &frame.blade,
        &s->target[BFM_BOULDER_ROOTRICK_THROWS], &rival_model, contact, normal);
    if (standing < 0) standing = frame.ordinary_standing;
    int events = bfm_woods_rootrick_finale_step(&s->actor, s->rival, standing, hit);
    if (events == BFM_ROOTRICK_FINALE_ERROR) return 0;
    out->release = !!(events & BFM_ROOTRICK_FINALE_RELEASE);
    out->shake = !!(events & BFM_ROOTRICK_FINALE_SHAKE);
    out->remove_shadow = !!(events & BFM_ROOTRICK_FINALE_REMOVE_SHADOW);
    if (events & BFM_ROOTRICK_FINALE_CRY) out->voice = 0x4331;
    out->sound = frame.blade.sound;
    if (s->rock) {
        BfmBoulderTarget hero_target = {hero->root.xyz, &hero_model, &s->target[BFM_BOULDER_MUSASHI_THROWS]};
        BfmBoulderTarget rival_target = {s->rival->xyz, &rival_model, &s->target[BFM_BOULDER_ROOTRICK_THROWS]};
        BfmBoulderEvent event = bfm_woods_rootrick_boulder_step(s->rock, &hero_target, &rival_target,
            s->rival->yaw, s->rival->clip, &out->impact);
        if (event == BFM_BOULDER_ERROR) return 0;
        if (event == BFM_BOULDER_STRUCK_MUSASHI || event == BFM_BOULDER_STRUCK_ROOTRICK) {
            out->struck_hero = event == BFM_BOULDER_STRUCK_MUSASHI;
            if (!bfm_woods_rootrick_finale_boulder_hit(&s->actor, !out->struck_hero)) return 0;
            out->voice = out->impact.voice;
        }
    }
    return 1;
}

int bfm_woods_rootrick_summit_visible(const BfmWoodsRootrickSummit *s)
{
    return s && s->actor.visible;
}

uint32_t bfm_woods_rootrick_summit_grants(const BfmWoodsRootrickSummit *s)
{
    return s ? s->grants : 0;
}

int bfm_woods_rootrick_summit_finish(BfmWoodsRootrickSummit *s, int defeat)
{
    return s && bfm_woods_rootrick_player_finish(s->player, defeat);
}

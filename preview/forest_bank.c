/* SPDX-License-Identifier: MIT */
#include "forest_bank.h"
#include "forest_maze_guide_start.h"
#include "forest_bank_hopper.h"
#include "forest_bank_thorns.h"
#include "forest_bank_fusion.h"
#include "forest_bank_hop_ring.h"
#include "forest_message_art.h"
#include "woods_award_effect.h"
#include "woods_shrink_message.h"
#include "woods_player_hit.h"
#include "woods_revive.h"
#include "woods_fusion_art.h"
#include "scene_tiles.h"
#include "village_field.h"
#include "village_field_defeat.h"
#include "village_lighting.h"
#include <stdlib.h>
#include <string.h>

struct BfmForestBank {
    BfmVillageField field;
    BfmVillageDaylight daylight;
    BfmBankHoppers *hoppers;
    BfmForestMazeGuideStart *guide;
    BfmWoodsFusion *fusion;
    BfmWoodsFusionFrame action;
    BfmForestBankHop hop;
    BfmForestBankHopPose *hop_pose;
    BfmForestBankHopRing hop_dust;
    BfmBedroomRoot launch_root;
    int hop_contact, hop_launch, hop_field;
    BfmWoodsAwardEffect apparition;
    const MusashiDiscMedia *disc;
    const uint8_t *scene;
    size_t bytes;
    unsigned last_action, award_sound, hop_sound;
    int targetable;
    int16_t aim[3];
    int award_open, guarding;
    uint32_t guard_pose, guard_parts;
    BfmWoodsPlayerHit *receiver;
    BfmWoodsPlayerHitFrame reaction;
    BfmForestBankThornHurt thorn;
    BfmForestBankThornFrame thorn_frame;
    BfmForestDamageDigits thorn_damage, thorn_credit;
    uint8_t damage_palette, credit_palette;
    enum { BANK_ALIVE, BANK_CHECK_STOCK, BANK_DEFEATED } life;
    BfmRetryScreen *retry;
    BfmVillageFieldDefeat defeat_actor;
    BfmForestMessageArt *writing;
    const uint8_t *main;
    size_t main_bytes;
    unsigned incoming, attack_yaw;
    uint16_t (*tiles)[1024];
};

void bfm_forest_bank_free(BfmForestBank *bank)
{
    if (bank) {
        bfm_forest_bank_hop_pose_free(bank->hop_pose);
        bfm_retry_screen_free(bank->retry);
        bfm_woods_fusion_free(bank->fusion);
        bfm_forest_message_art_free(bank->writing);
        bfm_bank_hoppers_free(bank->hoppers);
        bfm_forest_maze_guide_start_free(bank->guide);
        bfm_woods_player_hit_free(bank->receiver);
        bfm_village_field_clear(&bank->field);
        free(bank);
    }
}

BfmForestBank *bfm_forest_bank_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                   size_t bytes, uint16_t tiles[512][1024], unsigned phase)
{
    if (!disc || !scene || !tiles || phase >= 5) return NULL;
    BfmForestBank *bank = calloc(1, sizeof(*bank));
    if (!bank) return NULL;
    const size_t palette = UINT32_C(0x8018f745) - UINT32_C(0x80128158);
    bank->disc = disc;
    bank->scene = scene;
    bank->bytes = bytes;
    bank->action.tether_gauge = -1;
    if (!bfm_forest_bank_hop_init(&bank->hop,scene,bytes)) {
        bfm_forest_bank_free(bank);
        return NULL;
    }
    bank->fusion = bfm_forest_bank_fusion_open(disc,scene,bytes);
    bank->hoppers = bfm_bank_hoppers_open(scene,bytes);
    bank->guide = bfm_forest_maze_guide_start_open(scene,bytes);
    if (bytes > palette + 3) {
        bank->damage_palette = scene[palette];
        bank->credit_palette = scene[palette + 3];
        bank->receiver = bfm_woods_player_hit_open_palette(disc,bank->damage_palette);
    }
    if (!bank->fusion || !bank->hoppers || !bank->guide || !bank->receiver ||
        !bfm_village_field_load_bank(&bank->field, disc, scene, bytes)) {
        bfm_forest_bank_free(bank);
        return NULL;
    }
    /* Sparse uploads preserve resident tiles, including the field HUD. */
    uint16_t (*pending)[1024] = malloc(512 * sizeof(*pending));
    if (pending) {
        memcpy(pending, tiles, 512 * sizeof(*pending));
        if (bfm_scene_read_tiles(disc, "MAIN.CD", 10, pending) &&
            bfm_scene_read_tiles(disc, "SC03.CD", 6, pending) &&
            bfm_forest_bank_daylight_begin(&bank->daylight, scene, bytes, phase, pending)) {
            bank->writing = bfm_castle_message_art_load(disc, scene, bytes, pending, 0x64b58);
            if (bank->writing) {
                memcpy(tiles, pending, 512 * sizeof(*pending));
                bank->tiles = tiles;
            }
        }
        free(pending);
    }
    if (!bank->tiles) {
        bfm_forest_bank_free(bank);
        return NULL;
    }
    return bank;
}

void bfm_forest_bank_bind_player(BfmForestBank *bank, const uint8_t *main, size_t bytes)
{
    bank->main = main;
    bank->main_bytes = bytes;
}

void bfm_forest_bank_step_light(BfmForestBank *bank, unsigned phase)
{
    if (bank) bfm_village_daylight_step(&bank->daylight, phase, bank->tiles);
}

int bfm_forest_bank_receive(BfmForestBank *bank, BfmVillageMotion *motion,
                            BfmGameplayRun *run, BfmFieldInput input, unsigned actor_tick)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    if (!p) return 0;
    int active = !bfm_woods_fusion_awarding(bank->fusion);
    bank->targetable = active && p->controlled;
    if (!run) return 1;
    uint16_t *reason = bfm_village_motion_bank_pending(motion);
    if (!reason) return 0;
    BfmWoodsCasterRingHit incoming = {0};
    unsigned scene_state = bank->targetable ? 0u : 1u;
    int terrain_hit = bfm_forest_bank_thorn_hit(reason,scene_state,run->player.record[0x49],
                                                (uint16_t)p->root.yaw,&incoming);
    BfmForestBankThornInput terrain = {
        .incoming=terrain_hit ? &incoming : NULL, .disc=bank->disc, .field=actor_tick,
        .scene_state=scene_state, .player_action=bank->last_action,
        .player_flags=bank->action.blade_away ? 4u : 0u
    };
    bfm_forest_damage_digits_advance(&bank->thorn_damage);
    bfm_forest_damage_digits_advance(&bank->thorn_credit);
    if (!bfm_village_motion_bank_thorn(motion,&bank->thorn,run,terrain,&bank->thorn_frame))
        return 0;
    BfmForestBankThornFrame *thorn = &bank->thorn_frame;
    if (thorn->started) {
        bank->guarding = 0;
        if (thorn->damage_number) {
            bfm_forest_damage_digits_start_data(&bank->thorn_damage,&bank->damage_palette,
                                                thorn->effect_root,(uint16_t)thorn->damage_number);
            bank->thorn_damage.hurt = 1;
            bfm_forest_damage_digits_scatter(&bank->thorn_damage,bfm_gameplay_random(run));
        }
        if (thorn->bp_credit) {
            bfm_forest_damage_digits_start_data(&bank->thorn_credit,&bank->credit_palette,
                                                thorn->effect_root,(uint16_t)thorn->bp_credit);
            bank->thorn_credit.bp_credit = 1;
        }
    }
    /* Terrain emission replaces the actor's pending hit even if the current
     * action declines it. A suppressed thorn leaves the Hopper hit intact. */
    if (terrain_hit || thorn->movement_locked) bank->incoming = 0;
    BfmWoodsPlayerHitInput hit = {
        .root=&p->root, .airborne=!bfm_village_motion_grounded(motion),
        .hit_flags=bank->incoming ? 1u : 0u, .power=bank->incoming,
        .attacker_yaw=bank->attack_yaw, .receives_damage=active && p->controlled,
        .protected_action=active ? 0u : 1u, .player_action=bank->last_action,
        .guard_ready=bank->action.action == 17 && !(run->player.record[0x48] & 128),
        .player_flags=bank->action.blade_away ? 4u : 0u,
        .square_pressed=input.square_pressed, .triangle_pressed=input.triangle_pressed
    };
    bank->incoming = 0;
    bfm_woods_fusion_sample_charge(run,!thorn->movement_locked && !bank->action.blade_away &&
                                 (hit.guard_ready || bank->guarding),input.shoulder_held);
    if (!bfm_woods_player_hit_step(bank->receiver,run,&hit,&bank->reaction)) return 0;
    if (thorn->blink_fields) bfm_woods_player_hit_blink(bank->receiver,thorn->blink_fields);
    if (thorn->ended_ability == 12 ||
        (bank->reaction.hurt_started && run->player.record[0x49] == 12)) {
        memset(run->player.record+0x49,0,4);
        bank->hop.active = 0;
        bank->hop.cancelled_timer = 1;
        bfm_forest_bank_hop_pose_reset(bank->hop_pose);
    }
    if (hit.guard_ready && bank->reaction.hurt_started) run->player.record[0x47] = 0;
    if (bank->reaction.guard_started) {
        bank->guarding = 1;
        bank->guard_pose = bank->action.pose;
        bank->guard_parts = bank->action.visible;
    }
    if (!thorn->movement_locked && bank->reaction.movement_locked && !bank->reaction.zero_hp) {
        BfmWoodsReactionMotion kind = BFM_WOODS_HURT_HOLD;
        if (bank->guarding) kind = BFM_WOODS_GUARD_RECOIL;
        else if (bank->reaction.hurt_started) kind = BFM_WOODS_HURT_START;
        BfmWoodsMotionContact ground;
        if (!bfm_village_motion_woods_reaction(motion,NULL,kind,bank->reaction.recoil_x,
                                               bank->reaction.recoil_z,&ground)) return 0;
        BfmWoodsPlayerHitContact contact = BFM_WOODS_HIT_SAME_SUPPORT;
        if (ground.lost_support) contact = BFM_WOODS_HIT_LOST_SUPPORT;
        else if (ground.slope & 0x4000) contact = BFM_WOODS_HIT_SURFACE_REACTION;
        else if (ground.landed) contact = BFM_WOODS_HIT_LANDED;
        if (bank->guarding) bfm_woods_player_hit_guard_contact(bank->receiver,contact,&bank->reaction);
        else bfm_woods_player_hit_hurt_contact(bank->receiver,run,hit.player_flags,contact,&bank->reaction);
        if (bank->reaction.recovered && !bank->reaction.guard_cancel &&
            !bank->reaction.surface_reaction && !bank->reaction.solid_actor_reaction &&
            !bfm_village_motion_woods_recover(motion,bank->reaction.resume_fall)) return 0;
    }
    if (thorn->zero_hp || bank->reaction.zero_hp) {
        bank->life = BANK_CHECK_STOCK;
        return BFM_BANK_HIT_FATAL;
    }
    return 1;
}

int bfm_forest_bank_recovery_step(BfmForestBank *bank, BfmVillageMotion *motion,
                                  BfmGameplayRun *run, BfmSaveRecord *save,
                                  BfmForestMessage **opened)
{
    if (bank->life == BANK_ALIVE) {
        if (run) bfm_village_motion_woods_condition(motion, bfm_field_tired_stance(run->player.record));
        return BFM_BANK_PLAYING;
    }
    if (bank->life == BANK_DEFEATED) return BFM_BANK_WAITING;
    unsigned slot;
    BfmForestMessage *notice = NULL;
    BfmWoodsReviveResult result = bfm_woods_revive_prepare(run, save, 0, bank->main,
                                                bank->main_bytes, &slot, &notice);
    if (result == BFM_WOODS_REVIVE_ERROR) return BFM_BANK_RECOVERY_ERROR;
    if (result == BFM_WOODS_REVIVE_UNAVAILABLE) {
        bank->life = BANK_DEFEATED;
        return BFM_BANK_WAITING;
    }
    if (!opened || *opened ||
        !bfm_village_motion_woods_revive(motion, bfm_field_tired_stance(run->player.record))) {
        bfm_forest_message_free(notice);
        return BFM_BANK_RECOVERY_ERROR;
    }
    bfm_woods_revive_restore(run, save, slot);
    bfm_village_motion_woods_condition(motion, bfm_field_tired_stance(run->player.record));
    bank->life = BANK_ALIVE;
    *opened = notice;
    return BFM_BANK_REVIVED;
}

int bfm_forest_bank_begin_retry(BfmForestBank *bank, const MusashiDiscMedia *disc,
                                const uint8_t *scene, size_t scene_bytes,
                                const BfmVillageMotion *motion, const BfmSaveRecord *save,
                                const BfmVillageCamera *camera, const unsigned *age,
                                BfmGameplayRun *run)
{
    if (!bank || bank->life != BANK_DEFEATED || bank->retry) return 1;
    bank->defeat_actor = (BfmVillageFieldDefeat){
        .field = &bank->field, .motion = motion, .save = save, .main = bank->main,
        .main_bytes = bank->main_bytes, .tiles = bank->tiles, .age = age
    };
    const size_t table = UINT32_C(0x8018e950) - UINT32_C(0x80128158);
    bank->retry = bfm_village_field_defeat_open(&bank->defeat_actor, disc, scene, scene_bytes,
                                               table, camera, run);
    return bank->retry != NULL;
}

BfmRetryScreen *bfm_forest_bank_retry(const BfmForestBank *bank)
{
    return bank ? bank->retry : NULL;
}

void bfm_forest_bank_draw_message(const BfmForestBank *bank, const BfmForestMessageFrame *window,
                                  uint32_t pixels[240][320])
{
    if (window) bfm_forest_message_art_draw(bank->writing, window, pixels);
}

int bfm_forest_bank_draw_retry(BfmForestBank *bank, const BfmVillageMotion *motion,
                               const BfmVillageCamera *camera, const BfmForestHud *hud,
                               uint32_t pixels[240][320])
{
    if (!bank || !bank->retry) return 0;
    if (bfm_retry_screen_keep_world(bank->retry) &&
        !bfm_forest_bank_draw(bank, motion, camera, bank->defeat_actor.save,
                              *bank->defeat_actor.age, pixels)) return 0;
    return bfm_retry_screen_draw(bank->retry, hud, bank->writing, pixels);
}

int bfm_forest_bank_awarding(const BfmForestBank *bank)
{
    return bfm_woods_fusion_awarding(bank->fusion);
}

int bfm_forest_bank_movement_locked(const BfmForestBank *bank)
{
    return bank->thorn_frame.movement_locked || bank->reaction.movement_locked ||
           bank->action.movement_locked || bank->award_open;
}

unsigned bfm_forest_bank_sound(const BfmForestBank *bank)
{
    if (bank->thorn_frame.sound) return bank->thorn_frame.sound;
    if (bank->reaction.sound) return bank->reaction.sound;
    return bank->award_sound ? bank->award_sound : bank->hop_sound;
}

unsigned bfm_forest_bank_take_sound(BfmForestBank *bank)
{
    unsigned *pending = bank->thorn_frame.sound ? &bank->thorn_frame.sound : &bank->reaction.sound;
    if (!*pending) pending = bank->award_sound ? &bank->award_sound : &bank->hop_sound;
    unsigned sound = *pending;
    *pending = 0;
    return sound;
}

void bfm_forest_bank_step_actors(BfmForestBank *bank, const BfmBedroomRoot *player,
                                 BfmGameplayRun *run, BfmSaveRecord *save)
{
    if (bfm_woods_fusion_awarding(bank->fusion)) return;
    uint8_t stage = bfm_forest_maze_guide_start_step(bank->guide,player->xyz,save->variables[204]);
    if (stage != save->variables[204]) {
        save->variables[204] = stage;
        if (run) run->banks.variables[204] = stage;
    }
    /* A damaged player retains the last target point until their normal field
     * movement resumes. The feet still drive room actor admission and culling. */
    if (bank->targetable && bank->last_action != 25)
        memcpy(bank->aim,player->xyz,sizeof bank->aim);
    bank->incoming = bfm_bank_hoppers_step_target(bank->hoppers,player,
        bank->targetable ? bank->aim : NULL,run,&bank->attack_yaw);
}

int bfm_forest_bank_cast(BfmForestBank *bank, BfmVillageMotion *motion, BfmGameplayRun *run,
                         const BfmVillageCamera *camera, BfmSaveRecord *save, BfmFieldInput input,
                         unsigned field_tick, unsigned actor_tick, const int16_t previous[3],
                         BfmForestMessage **opened)
{
    if (!run) return 1;
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    int reacting = bank->thorn_frame.movement_locked || bank->reaction.movement_locked;
    bfm_forest_bank_hop_ring_step(&bank->hop_dust,&bank->launch_root,camera->eye,
                                  bank->hop_contact,bank->hop_launch);
    int return_guard = bank->guarding && bank->reaction.recovered && !bank->reaction.guard_cancel &&
                       !bank->reaction.resume_fall && input.shoulder_held;
    BfmWoodsFusionInput cast = {
        .actor_tick=actor_tick, .charge_sampled=1, .body_interrupted=reacting && !return_guard,
        .controllable=p->controlled && (!reacting || return_guard) && !bank->award_open && !bank->hop_field &&
                      !bank->reaction.miniature && bfm_village_motion_grounded(motion),
        .moving=p->root.xyz[0] != previous[0] || p->root.xyz[2] != previous[2],
        .shoulder=!bank->hop_field && input.shoulder_held,
        .square_edge=!reacting && !bank->hop_field && input.square_pressed,
        .visible=bfm_village_field_visible(&p->pose,save->flags), .eye=camera->eye
    };
    if (!bfm_forest_bank_fusion_step(bank->fusion,bank->hoppers,run,&p->root,field_tick,
                                     &cast,&bank->action)) return 0;
    if ((bank->action.award_face_slot || bank->action.award_finished) &&
        !bfm_village_motion_ability_pose(motion,bank->action.award_face_slot,
                                        bank->action.award_finished)) return 0;
    if (bank->reaction.recovered) bank->guarding = 0;
    if (bank->thorn_frame.movement_locked)
        bank->last_action = bank->thorn_frame.action;
    else if (reacting && !bank->reaction.recovered)
        bank->last_action = bank->guarding ? 27u : 25u;
    else
        bank->last_action = (unsigned)bank->action.action;
    memcpy(save->progress,run->player.record,sizeof save->progress);
    if (bank->action.open_award) {
        if (!opened || *opened) return 0;
        *opened = bfm_woods_ability_message_open(bank->scene,bank->bytes,bank->main,
                                                bank->main_bytes,bank->action.award_ability);
        if (!*opened) return 0;
        bank->award_open = 1;
    }
    if (bank->action.release_award) bank->award_sound = 2441;
    unsigned released = bank->action.release_award ? bank->action.award_ability : 0;
    return bfm_woods_award_effect_step_ability(&bank->apparition,bank->disc,bank->scene,
                                              bank->bytes,bank->tiles,released);
}

void bfm_forest_bank_hop_sound(BfmForestBank *bank)
{
    bank->hop_sound = 0x47a;
}

int bfm_forest_bank_move(BfmForestBank *bank, BfmVillageMotion *motion,
                         BfmGameplayRun *run, BfmFieldInput field, int camera_yaw)
{
    bank->hop_contact = bank->hop_launch = bank->hop_field = 0;
    if (bfm_forest_bank_movement_locked(bank)) return 1;
    if (run && bfm_forest_bank_body_available(bank)) {
        if (run->player.record[0x49] == 12) {
            if (!bank->hop_pose) bank->hop_pose = bfm_forest_bank_hop_pose_open(bank->disc);
            if (!bank->hop_pose || (!bank->hop_dust.dust.atlas.width &&
                !bfm_forest_bank_hop_ring_load(&bank->hop_dust,bank->disc))) return 0;
        }
        int was_hopping = bank->hop.active;
        if (!was_hopping) bank->launch_root = bfm_village_motion_player(motion)->root;
        int consumed = bfm_village_motion_bank_hop(motion,&bank->hop,bank->hop_pose,
            &run->player,field,camera_yaw,bank->action.blade_away ? 4u : 0u,&bank->hop_launch);
        if (consumed < 0) return 0;
        if (consumed) {
            bank->hop_contact = was_hopping && bank->hop_launch;
            if (bank->hop_contact) bank->launch_root = bfm_village_motion_player(motion)->root;
            bank->hop_field = 1;
            if (bank->hop_launch) bfm_forest_bank_hop_sound(bank);
            return 1;
        }
    }
    return bfm_village_motion_step_field(motion,field.x,field.y,camera_yaw,run);
}

int16_t bfm_forest_bank_tracking_y(const BfmForestBank *bank, int16_t foot_y)
{
    return bfm_forest_bank_hop_camera_y(&bank->hop,foot_y);
}

int bfm_forest_bank_body_available(const BfmForestBank *bank)
{
    return bank->life == BANK_ALIVE && !bank->thorn_frame.movement_locked && !bank->reaction.movement_locked &&
           !bank->action.action && !bank->action.blade_away && !bank->award_open;
}

int bfm_forest_bank_close(BfmForestBank *bank)
{
    if (!bank->award_open) return 1;
    if (!bfm_woods_fusion_close_award(bank->fusion)) return 0;
    bank->award_open = 0;
    return 1;
}

int bfm_forest_bank_tether(const BfmForestBank *bank)
{
    return bank->action.tether_gauge;
}

int bfm_forest_bank_award_cue(const BfmForestBank *bank)
{
    return bank->action.release_award;
}

int bfm_forest_bank_draw(BfmForestBank *bank, const BfmVillageMotion *motion,
                         const BfmVillageCamera *camera, const BfmSaveRecord *save,
                         unsigned age, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!bank || !player || !camera || !save || !pixels) return 0;
    BfmBedroomPose displayed = player->pose;
    if (!bank->thorn_frame.movement_locked &&
        !bfm_woods_fusion_art_pose(bank->main,bank->main_bytes,bank->action.pose,
                                  bank->action.visible,bank->action.blade_away,&displayed)) return 0;
    uint32_t parts = bfm_village_field_visible(&displayed,save->flags);
    uint32_t hurt_pose = bank->guarding ? bank->guard_pose : bank->reaction.pose;
    if (!bank->thorn_frame.movement_locked && hurt_pose &&
        !bfm_woods_fusion_art_pose(bank->main,bank->main_bytes,hurt_pose,
            bank->guarding ? bank->guard_parts : parts,bank->action.blade_away,&displayed)) return 0;
    parts = bfm_village_field_visible(&displayed,save->flags);
    int hidden = bank->retry || bank->thorn_frame.hide_body ||
                 (!bank->thorn_frame.movement_locked && bank->reaction.hide_body);
    if (!bfm_village_motion_face(motion, bank->tiles) ||
        !bfm_bedroom_figure_lights(bank->field.hero, bank->daylight.lamps) ||
        !bfm_village_field_prepare_scaled(&bank->field,camera,&player->root,&displayed,save->flags,
                                   age,hidden ? 0 : parts,4096,bank->tiles,&pixels[0][0])) return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(bank->field.field);
    if (!bfm_forest_maze_guide_start_draw(bank->guide,bank->field.hero,view) ||
        !bfm_bank_hoppers_draw(bank->hoppers,bank->field.hero,view) ||
        (!bank->retry && (
            !bfm_forest_bank_hop_ring_append(&bank->hop_dust,bank->field.hero,view) ||
            !bfm_woods_fusion_art_append(bank->field.hero,view,NULL,NULL,bank->reaction.digits) ||
            !bfm_woods_fusion_art_append(bank->field.hero,view,NULL,NULL,&bank->thorn_damage) ||
            !bfm_woods_fusion_art_append(bank->field.hero,view,NULL,NULL,&bank->thorn_credit) ||
            !bfm_woods_fusion_art_append(bank->field.hero,view,bank->action.blade,bank->action.trails,
                                     bank->action.bp_digits) ||
            !bfm_woods_fusion_art_lights(bank->field.hero,view,&bank->action) ||
            !bfm_woods_award_effect_append(&bank->apparition,bank->field.hero,view)))) return 0;
    /* This room has no panorama or sky bands. Terrain and actor faces share
     * a depth ordering over the cleared scanout. */
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    bfm_bedroom_field_draw(bank->field.field, bank->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, bank->field.hero);
    return 1;
}

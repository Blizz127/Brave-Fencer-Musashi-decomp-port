/* SPDX-License-Identifier: MIT */
#include "southern_woods.h"
#include "woods_water.h"
#include "woods_water_effect.h"
#include "village_field.h"
#include "village_field_defeat.h"
#include "village_arrival.h"
#include "village_clock.h"
#include "village_lighting.h"
#include "village_sky.h"
#include "scene_tiles.h"
#include "forest_message_art.h"
#include "woods_sign_model.h"
#include "woods_lardwick_field.h"
#include "woods_lardwick_art.h"
#include "woods_lardwick_player.h"
#include "village_talk_camera.h"
#include "woods_caster.h"
#include "woods_caster_loot.h"
#include "woods_caster_burst.h"
#include "woods_caster_melee.h"
#include "woods_charged_fx.h"
#include "woods_charged_targets.h"
#include "woods_player_hit.h"
#include "woods_revive.h"
#include "woods_mini_smoke.h"
#include "woods_satiate_effect.h"
#include "woods_carry.h"
#include "woods_leno.h"
#include "woods_award_effect.h"
#include "woods_fusion.h"
#include "woods_fusion_art.h"
#include "woods_shrink_message.h"
#include "woods_shrink_cast.h"
#include "woods_king.h"
#include "woods_king_reward.h"
#include "woods_body_notice.h"
#include "woods_lily.h"
#include "bedroom_effects.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>

struct BfmSouthernWoods {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    BfmVillageMotion *motion;
    BfmVillageField field;
    BfmVillageCamera camera, presented_camera;
    BfmVillageDaylight daylight;
    BfmWoodsWater water;
    BfmForestWaterEffect splash;
    BfmForestDamageDigits water_digits;
    int16_t water_contact[3];
    int in_water;
    uint16_t (*tiles)[1024];
    BfmWoodsSignModel *sign;
    BfmWoodsLardwickField lardwick;
    BfmWoodsLardwickArt *lardwick_art;
    BfmWoodsLardwickPlayer rescue_player;
    BfmVillageTalkCamera rescue_camera;
    int rescue_awarding;
    BfmWoodsCaster *caster;
    BfmWoodsCasterLoot *loot;
    BfmWoodsCasterBurst *burst;
    BfmWoodsCasterMelee *melee;
    BfmWoodsChargedFx *charged_fx;
    BfmWoodsChargedImpact charged_impact;
    BfmWoodsCasterMeleeFrame swing;
    BfmWoodsPlayerHit *receiver;
    BfmWoodsMiniSmoke mini_smoke;
    BfmWoodsSatiateEffect *satiate;
    BfmWoodsCasterRingHit incoming;
    BfmWoodsPlayerHitFrame reaction;
    BfmRetryScreen *retry;
    BfmVillageFieldDefeat defeat_actor;
    unsigned last_action;
    int guarding, guard_swing, cut_sounded_surface;
    enum { ALIVE, CHECK_STOCK, DEFEAT_PENDING, DEFEAT_REPORTED } life;
    uint32_t guard_pose, guard_parts;
    BfmWoodsLenoActor *leno_art;
    BfmWoodsLeno *leno;
    BfmWoodsCarry carry;
    BfmWoodsKing *king;
    BfmWoodsLilies *lilies;
    BfmWoodsKingReward king_reward;
    BfmWoodsBodyNotice body_notice;
    BfmWoodsFusion *fusion;
    BfmWoodsFusionFrame action;
    BfmSteamChargeEffects charge;
    BfmWoodsFootDust foot;
    BfmWoodsChargeArt charge_art;
    BfmWoodsShrinkCast *shrink;
    BfmWoodsShrinkFrame spell;
    unsigned sound, commands[BFM_VILLAGE_WOODS_SOUNDS];
    size_t command_count;
    BfmWoodsAwardEffect apparition;
    const MusashiDiscMedia *disc;
    BfmSaveRecord *save;
    const uint8_t *main;
    size_t main_bytes;
    int award_open;
    BfmForestMessageArt *writing;
    const uint8_t *sign_text;
    size_t sign_bytes;
    unsigned age;
};

static void follow(BfmSouthernWoods *woods, int entering)
{
    const BfmBedroomRoot *root = &bfm_village_motion_player(woods->motion)->root;
    BfmVillageCamera *cam = &woods->camera;
    const int32_t origin[3] = {0}, position[3] = {root->xyz[0], 0, root->xyz[2]};
    for (unsigned axis = 0; axis < 3; ++axis) {
        int target = root->xyz[axis] - (axis == 1 ? 128 : 0);
        int difference = target - cam->target[axis];
        int step = difference / 4;
        if (!step && difference) step = difference < 0 ? -1 : 1;
        cam->target[axis] = entering ? target : cam->target[axis] + step;
    }
    int bearing = bfm_village_motion_view_yaw(origin,position);
    bfm_village_camera_woods_orbit(cam, bearing, root->xyz[2], entering);
}

void bfm_southern_woods_free(BfmSouthernWoods *woods)
{
    if (!woods) return;
    bfm_retry_screen_free(woods->retry);
    bfm_forest_message_art_free(woods->writing);
    bfm_woods_fusion_free(woods->fusion);
    bfm_woods_shrink_cast_free(woods->shrink);
    bfm_woods_king_free(woods->king);
    bfm_woods_lilies_free(woods->lilies);
    bfm_woods_leno_free(woods->leno);
    bfm_woods_leno_actor_free(woods->leno_art);
    bfm_woods_satiate_effect_free(woods->satiate);
    bfm_woods_player_hit_free(woods->receiver);
    bfm_woods_charged_fx_free(woods->charged_fx);
    bfm_woods_caster_melee_free(woods->melee);
    bfm_woods_caster_burst_free(woods->burst);
    bfm_woods_caster_loot_free(woods->loot);
    bfm_woods_caster_free(woods->caster);
    bfm_woods_lardwick_art_free(woods->lardwick_art);
    bfm_woods_sign_model_free(woods->sign);
    bfm_village_motion_free(woods->motion);
    bfm_village_field_clear(&woods->field);
    free(woods);
}

BfmSouthernWoods *bfm_southern_woods_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *door, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    BfmBedroomEntry before = *entry, next = before;
    if (!bfm_castle_room_entry(disc,door,&next) || next.scene_member != 29) return NULL;
    BfmSouthernWoods *woods = calloc(1,sizeof *woods);
    if (!woods) return NULL;
    woods->bytes = bfm_read_archive_field_scene(disc,"SC03.CD",next.scene_member,woods->scene);
    if (door[2] == 1 || door[2] == 3)
        woods->motion = bfm_village_motion_box_field(woods->scene,woods->bytes,main,main_bytes,door);
    else
        woods->motion = bfm_village_motion_woods(woods->scene,woods->bytes,main,main_bytes);
    if (!bfm_village_motion_trail_gate(woods->motion,u32le(next.state.progress+8))) goto fail;
    if (!woods->motion || !bfm_village_field_load_woods(&woods->field,disc,woods->scene,woods->bytes) ||
        !bfm_scene_read_tiles(disc,"MAIN.CD",10,tiles) ||
        !bfm_scene_read_tiles(disc,"SC03.CD",next.scene_member,tiles)) goto fail;
    if (!bfm_woods_lardwick_field_open(&woods->lardwick, woods->scene, woods->bytes,
                                       next.state.variables)) goto fail;
    if (woods->lardwick.active) {
        woods->lardwick_art = bfm_woods_lardwick_art_load(woods->scene, woods->bytes, &woods->lardwick);
        if (!woods->lardwick_art) goto fail;
    }
    bfm_woods_water_load(&woods->water, tiles);
    if (!bfm_woods_daylight_begin(&woods->daylight, woods->scene, woods->bytes,
                                  next.state.progress[0x37], tiles)) goto fail;
    const uint32_t base = UINT32_C(0x80128158);
    const size_t script = UINT32_C(0x8018bdec) - base;
    if (woods->bytes < script + 20) goto fail;
    const uint8_t *commands = woods->scene + script;
    if (u32le(commands) != 10 || u32le(commands+4) || u32le(commands+8) != 1 ||
        u32le(commands+16) != 2) goto fail;
    uint32_t address = u32le(commands + 12);
    if (address < base || address-base > woods->bytes-4) goto fail;
    woods->sign_text = woods->scene + (address-base);
    size_t available = woods->bytes - (address-base) - 4;
    /* Three rows of 36 cells, with two line breaks and a terminator. */
    const size_t capacity = 3*36 + 3;
    const uint8_t *end = memchr(woods->sign_text+4,0,available < capacity ? available : capacity);
    if (!end || memcmp(woods->sign_text,(const uint8_t[]){5,2,1,1},4)) goto fail;
    woods->sign_bytes = (size_t)(end - woods->sign_text) + 1;
    woods->sign = bfm_woods_sign_model_load(woods->scene,woods->bytes,UINT32_C(0x8018ea38));
    woods->writing = bfm_southern_woods_message_art_load(disc,woods->scene,woods->bytes,tiles);
    woods->caster = bfm_woods_caster_load(woods->scene, woods->bytes);
    woods->loot = bfm_woods_caster_loot_open(woods->scene, woods->bytes);
    woods->leno_art = bfm_woods_leno_actor_open(woods->scene, woods->bytes);
    woods->burst = bfm_woods_caster_burst_open(woods->scene, woods->bytes, main, main_bytes);
    woods->charged_fx = bfm_woods_charged_fx_open(woods->scene, woods->bytes, main, main_bytes);
    if (!woods->charged_fx || !bfm_woods_charged_impact_load(&woods->charged_impact,
                                                           woods->scene, woods->bytes)) goto fail;
    woods->melee = bfm_woods_caster_melee_open(disc, woods->scene, woods->bytes, main, main_bytes);
    woods->satiate = bfm_woods_satiate_effect_load(disc);
    woods->receiver = bfm_woods_player_hit_open(disc, woods->scene, woods->bytes);
    if (!woods->satiate || !woods->loot || !woods->burst || !woods->melee || !woods->receiver ||
        !woods->leno_art || !bfm_woods_charge_core_load(&woods->charge_art.core, woods->scene, woods->bytes) ||
        !bfm_woods_foot_dust_load(&woods->foot, disc) ||
        !bfm_woods_mini_smoke_load(&woods->mini_smoke, main, main_bytes) ||
        !bfm_woods_carry_load(&woods->carry, disc, woods->scene, woods->bytes)) goto fail;
    woods->fusion = bfm_woods_fusion_open(disc, woods->scene, woods->bytes);
    woods->king = bfm_woods_king_open(woods->scene, woods->bytes, next.state.flags);
    woods->lilies = bfm_woods_lilies_open(disc, woods->scene, woods->bytes);
    woods->shrink = bfm_woods_shrink_cast_open(disc, woods->scene, woods->bytes);
    if (!woods->sign || !woods->writing || !woods->caster || !woods->fusion ||
        !woods->king || !woods->lilies || !woods->shrink ||
        !bfm_woods_water_effect_load(&woods->splash, woods->scene, woods->bytes) ||
        !bfm_woods_king_reward_load(&woods->king_reward, disc, main, main_bytes) ||
        !bfm_woods_body_notice_load(&woods->body_notice, disc, woods->scene, woods->bytes)) goto fail;
    woods->disc = disc;
    woods->main = main;
    woods->main_bytes = main_bytes;
    woods->save = &entry->state;
    woods->action.tether_gauge = -1;
    woods->tiles = tiles;
    woods->camera.focal = 400;
    follow(woods,1);
    woods->presented_camera = woods->camera;
    *entry = next;
    woods->leno = bfm_woods_leno_open(woods->scene, woods->bytes, woods->save,
                                     bfm_village_motion_ground(woods->motion), woods->leno_art);
    if (!woods->leno) {
        *entry = before;
        goto fail;
    }
    return woods;
fail:
    bfm_southern_woods_free(woods);
    return NULL;
}

const BfmVillageMotion *bfm_southern_woods_departure(const BfmSouthernWoods *woods)
{
    return woods && bfm_village_motion_transfer(woods->motion) ? woods->motion : NULL;
}

const BfmVillagePlayer *bfm_southern_woods_player(const BfmSouthernWoods *woods)
{
    return bfm_village_motion_player(woods->motion);
}

int bfm_southern_woods_carrying_leno(const BfmSouthernWoods *woods)
{
    return woods && bfm_woods_leno_held(woods->leno);
}

int bfm_southern_woods_take_zero_hp(BfmSouthernWoods *woods)
{
    if (!woods || woods->life != DEFEAT_PENDING) return 0;
    woods->life = DEFEAT_REPORTED;
    return 1;
}

int bfm_southern_woods_begin_retry(BfmSouthernWoods *woods, BfmGameplayRun *run)
{
    if (!woods || woods->life != DEFEAT_PENDING) return 1;
    woods->defeat_actor = (BfmVillageFieldDefeat){
        .field = &woods->field, .motion = woods->motion, .save = woods->save,
        .main = woods->main, .main_bytes = woods->main_bytes,
        .tiles = woods->tiles, .age = &woods->age
    };
    const size_t table = UINT32_C(0x80189b80) - UINT32_C(0x80128158);
    woods->retry = bfm_village_field_defeat_open(&woods->defeat_actor, woods->disc,
                       woods->scene, woods->bytes, table, &woods->presented_camera, run);
    if (!woods->retry) return 0;
    return bfm_southern_woods_take_zero_hp(woods);
}

BfmRetryScreen *bfm_southern_woods_retry(const BfmSouthernWoods *woods)
{
    return woods ? woods->retry : NULL;
}

int bfm_southern_woods_prepare_recovery(const BfmSouthernWoods *woods, const BfmGameplayRun *run,
                                       BfmBedroomEntry *out)
{
    if (!woods || !woods->retry) return 0;
    return bfm_woods_retry_destination(woods->disc, run,
        bfm_retry_screen_woods_destination(woods->retry), woods->scene, woods->bytes, out);
}

int bfm_southern_woods_draw_retry(BfmSouthernWoods *woods, const BfmForestHud *hud,
                                  uint32_t pixels[240][320])
{
    if (!woods || !woods->retry) return 0;
    if (bfm_retry_screen_keep_world(woods->retry) &&
        !bfm_southern_woods_draw(woods, woods->save->flags, NULL, pixels)) return 0;
    return bfm_retry_screen_draw(woods->retry, hud, woods->writing, pixels);
}

int bfm_southern_woods_close(BfmSouthernWoods *woods)
{
    if (woods->lardwick.window)
        return bfm_woods_lardwick_close(&woods->lardwick);
    if (bfm_woods_leno_in_dialogue(woods->leno))
        return bfm_woods_leno_close_message(woods->leno);
    if (woods->award_open) {
        woods->award_open = 0;
        return bfm_woods_fusion_close_award(woods->fusion);
    }
    return bfm_village_motion_end_read(woods->motion);
}

int bfm_southern_woods_award_cue(const BfmSouthernWoods *woods)
{
    return woods->action.release_award;
}

unsigned bfm_southern_woods_sound(const BfmSouthernWoods *woods)
{
    return woods->sound;
}

static void room_sound(BfmSouthernWoods *woods, unsigned id)
{
    if (!id) return;
    woods->sound = id;
    if (woods->command_count < BFM_VILLAGE_WOODS_SOUNDS)
        woods->commands[woods->command_count++] = id;
}

size_t bfm_southern_woods_take_sounds(BfmSouthernWoods *woods, unsigned *out, size_t capacity)
{
    if (!woods || !out || !capacity) return 0;
    size_t count = woods->command_count < capacity ? woods->command_count : capacity;
    memcpy(out, woods->commands, count * sizeof *out);
    woods->command_count -= count;
    memmove(woods->commands, woods->commands + count, woods->command_count * sizeof *out);
    return count;
}

int bfm_southern_woods_tether_gauge(const BfmSouthernWoods *woods)
{
    return woods->action.tether_gauge;
}

static int in_sign_reach(const BfmSouthernWoods *woods, const int16_t position[3])
{
    const int16_t *point = bfm_woods_sign_model_position(woods->sign);
    int64_t distance = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int delta = position[axis] - point[axis];
        distance += (int64_t)delta * delta;
    }
    return distance <= 64 * 64;
}

static int carry_action(BfmSouthernWoods *woods, BfmGameplayRun *run, BfmFieldInput input,
                        int can_lift, BfmWoodsCarryFrame *frame)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(woods->motion);
    int held = bfm_woods_carry_active(&woods->carry);
    if (!run || (!held && (!can_lift || !woods->carry.braced ||
                           !(input.square_pressed || input.triangle_pressed)))) return 0;
    int lumina = (woods->save->flags[128 / 8] & 1) != 0;
    BfmForestTransform root;
    if (!bfm_bedroom_figure_actor_root(&player->root, player->pose.pitch, &root)) return -1;
    if (held) {
        return bfm_woods_carry_step(&woods->carry, run, &root, input.x || input.y,
                                   input.square_pressed, input.triangle_pressed, lumina, frame) ? 1 : -1;
    }
    int16_t end[3];
    if (!bfm_woods_carry_reach(&woods->carry, &root, input.square_pressed, input.triangle_pressed,
                              lumina, end) ||
        !bfm_woods_leno_target(woods->leno, player->root.xyz, end)) return 0;
    return bfm_woods_carry_begin(&woods->carry, run, frame) ? 1 : -1;
}

static int dog_contact(BfmSouthernWoods *woods, const int16_t previous[3], int16_t endpoint[3])
{
    if (previous[1] < INT16_MIN + 16 || endpoint[1] < INT16_MIN + 16) return 0;
    int16_t from[3] = {previous[0], previous[1] - 16, previous[2]};
    int16_t to[3] = {endpoint[0], endpoint[1] - 16, endpoint[2]}, resolved[3];
    if (!bfm_woods_leno_brace(woods->leno, from, to, resolved) || resolved[1] > INT16_MAX - 16)
        return 0;
    endpoint[0] = resolved[0];
    endpoint[1] = resolved[1] + 16;
    endpoint[2] = resolved[2];
    return 1;
}

static int open_field_message(BfmForestMessage **opened, BfmForestMessage *page)
{
    if (!page) return 0;
    if (!opened || (*opened && !bfm_field_message_keeps_control(*opened))) {
        bfm_forest_message_free(page);
        return 0;
    }
    bfm_forest_message_free(*opened);
    *opened = page;
    return 1;
}

static int update_dog(BfmSouthernWoods *woods, BfmGameplayRun *run,
                      const BfmWoodsCarryFrame *carry, BfmForestMessage **opened)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(woods->motion);
    BfmWoodsLenoInput input = {.player=p->root.xyz, .player_yaw=(int)p->root.yaw,
                               .grip=carry->attached ? carry->socket : NULL};
    switch (carry->event) {
    case BFM_CARRY_LIFT: input.command = BFM_LENO_LIFT; break;
    case BFM_CARRY_THROW: input.command = BFM_LENO_THROW; break;
    case BFM_CARRY_DROP: input.command = BFM_LENO_DROP; break;
    default: break;
    }
    if (run && !input.command && bfm_woods_leno_random_due(woods->leno))
        input.random = bfm_gameplay_random(run);
    BfmWoodsLenoEffects effects;
    BfmForestMessage *dialogue = NULL;
    if (!bfm_woods_leno_step(woods->leno, &input, &dialogue, &effects)) return 0;
    if (dialogue && !open_field_message(opened, dialogue)) return 0;
    int picture[3];
    if (effects.sound[0] != BFM_WOODS_LENO_BARK_SOUND ||
        (bfm_bedroom_project(bfm_bedroom_field_view(woods->field.field), effects.effect_xyz, picture) &&
         abs(picture[0] - 160) < 200 && abs(picture[1] - 120) < 170))
        room_sound(woods, effects.sound[0]);
    room_sound(woods, effects.sound[1]);
    return 1;
}

static int step_charged_effects(BfmSouthernWoods *woods, const BfmVillagePlayer *player)
{
    const BfmWoodsCasterMeleeFrame *cut = &woods->swing;
    BfmForestSweepEdge edge;
    if (cut->selected_pose) {
        BfmBedroomPose selected = player->pose;
        BfmForestTransform model;
        if (!bfm_woods_fusion_art_pose(woods->main, woods->main_bytes, cut->selected_pose, 0, 0, &selected) ||
            !bfm_bedroom_figure_actor_root(&player->root, selected.pitch, &model)) return 0;
        int8_t anchors[2][3];
        memcpy(anchors, selected.pose + 8, sizeof anchors);
        if (!bfm_forest_sweep_edge(&model, anchors, &edge)) return 0;
    }
    return bfm_woods_charged_fx_step(woods->charged_fx, cut->charge_started, cut->charge_marker,
                                    cut->action == 19, cut->selected_pose ? &edge : NULL);
}

static void charged_rewards(BfmSouthernWoods *woods, const BfmWoodsChargedResult *result)
{
    room_sound(woods, result->leno_yelp);
    bfm_woods_weapon_notice_start(&woods->body_notice, result->levels.lumina, result->levels.fusion);
    if (result->levels.lumina) room_sound(woods, BFM_BODY_LEVEL_SOUND);
    if (result->levels.fusion) room_sound(woods, BFM_BODY_LEVEL_SOUND);
}

static int charged_contacts(BfmSouthernWoods *woods, BfmGameplayRun *run,
                            unsigned heading, const BfmBedroomView *view)
{
    const BfmWoodsCasterMeleeFrame *swing = &woods->swing;
    const BfmWoodsChargedWeapon *weapon = bfm_woods_caster_melee_weapon(woods->melee);
    BfmWoodsChargedTargets targets = {
        .casters=woods->caster, .king=woods->king, .leno=woods->leno,
        .ground=bfm_village_motion_ground(woods->motion), .lardwick=&woods->lardwick
    };
    BfmWoodsChargedResult result;
    if (swing->charge_started) woods->cut_sounded_surface = 0;
    if (swing->event == 11) {
        for (unsigned ray = 0; ray < swing->rays; ++ray) {
            if (!bfm_woods_charged_blade_contact(&targets, &weapon->hit, weapon->ranks,
                    run, weapon->credit, heading, swing->strength, swing->from,
                    swing->to[ray], &result)) return 0;
            if (result.contacts) {
                if (result.freed_captive) room_sound(woods, BFM_WOODS_CRYSTAL_HIT_SOUND);
                if (!bfm_woods_charged_impact_issue(&woods->charged_impact,
                        result.point, result.normal, view->eye)) return 0;
                room_sound(woods, result.impact_sound);
                charged_rewards(woods, &result);
            } else if (result.terrain) {
                if (!bfm_woods_charged_impact_terrain(&woods->charged_impact,
                        result.terrain_point, result.terrain_normal, 0)) return 0;
                if (!woods->cut_sounded_surface) {
                    room_sound(woods, weapon->hit.surface_sound[result.material >> 13]);
                    woods->cut_sounded_surface = 1;
                }
            }
        }
    }
    int16_t center[3];
    if (bfm_woods_charged_fx_wave_center(woods->charged_fx, center)) {
        if (!bfm_woods_charged_wave_contact(&targets, &weapon->hit, weapon->ranks,
                run, weapon->credit, bfm_woods_charged_fx_wave_yaw(woods->charged_fx),
                center, &result)) return 0;
        charged_rewards(woods, &result);
    }
    return 1;
}

static void sword_contacts(BfmSouthernWoods *woods, unsigned yaw)
{
    const BfmWoodsCasterMeleeFrame *swing = &woods->swing;
    int connected = 0;
    for (unsigned ray = 0; ray < swing->rays; ++ray) {
        int16_t hit[3], end[3], normal[3];
        uint16_t material;
        memcpy(end, swing->to[ray], sizeof end);
        bfm_village_ground_blade(bfm_village_motion_ground(woods->motion), swing->from,
                                 end, normal, &material);
        if (bfm_woods_lardwick_contact(&woods->lardwick, swing->event, swing->from, end, hit, NULL) &&
            bfm_woods_lardwick_strike(&woods->lardwick, swing->event)) {
            memcpy(end, hit, sizeof end);
            bfm_woods_caster_melee_connected(woods->melee);
            room_sound(woods, BFM_WOODS_CRYSTAL_HIT_SOUND);
        }
        for (unsigned target = 1; target <= BFM_WOODS_CASTERS; ++target) {
            if (bfm_woods_caster_sword(woods->caster, target, swing->event, swing->strength, yaw,
                                      swing->from, end, hit)) {
                connected = 1;
                memcpy(end, hit, sizeof end);
            }
        }
    }
    if (connected) {
        bfm_woods_caster_melee_connected(woods->melee);
        room_sound(woods, swing->event == 4 ? BFM_WOODS_FUSION_FINISH_HIT_SOUND : BFM_FOREST_HIT_SOUND);
    }
}

static int caster_rewards(BfmSouthernWoods *woods, BfmGameplayRun *run)
{
    for (unsigned target = 1; target <= BFM_WOODS_CASTERS; ++target) {
        BfmBedroomRoot origin;
        if (bfm_woods_caster_burst(woods->caster, target, &origin) &&
            !bfm_woods_caster_burst_start(woods->burst, &origin, run)) return 0;
        int16_t position[3];
        unsigned experience = 0;
        if (!bfm_woods_caster_defeat(woods->caster, target, position, &experience)) continue;
        if (!bfm_woods_caster_loot_drop(woods->loot, run, position)) return 0;
        memcpy(woods->save->progress, run->player.record, sizeof woods->save->progress);
        unsigned level = bfm_woods_caster_reward_apply(&woods->king_reward, experience, woods->save);
        memcpy(run->player.record, woods->save->progress, sizeof run->player.record);
        memcpy(run->banks.figure_prices, woods->save->figure_prices, sizeof run->banks.figure_prices);
        if (level) {
            bfm_woods_body_notice_start(&woods->body_notice, level);
            room_sound(woods, BFM_BODY_LEVEL_SOUND);
        }
    }
    return 1;
}

static void queue_enemy_contact(BfmSouthernWoods *woods, const BfmWoodsKingEvent *king)
{
    /* The next player update receives a single hit. On the ordinary west-bank
     * approach the fixed King precedes the roaming enemy, then effects run. */
    const BfmWoodsCasterRingHit *ring = bfm_woods_caster_spell(woods->caster);
    unsigned strike = bfm_woods_caster_west_strike(woods->caster);
    if (ring) {
        woods->incoming = *ring;
    } else if (strike) {
        woods->incoming = (BfmWoodsCasterRingHit){
            .flags=1, .power=strike, .yaw=bfm_woods_caster_west_strike_yaw(woods->caster)
        };
    } else if (king->bite_power) {
        woods->incoming = (BfmWoodsCasterRingHit){
            .flags=1, .power=king->bite_power, .yaw=king->bite_yaw
        };
    }
}

static int receive_contact(BfmSouthernWoods *woods, BfmGameplayRun *run,
                           BfmFieldInput input, int active)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(woods->motion);

    BfmWoodsPlayerHitInput hit = {
        .root=&p->root, .player_action=woods->last_action,
        .player_flags=woods->action.blade_away ? 4u : 0u,
        .receives_damage=active && p->controlled, .protected_action=active ? 0u : 1u,
        .guard_ready=woods->action.action == 17 && !(run->player.record[0x48] & 128),
        .airborne=!bfm_village_motion_grounded(woods->motion),
        .square_pressed=input.square_pressed, .triangle_pressed=input.triangle_pressed,
        .carrying=bfm_woods_leno_held(woods->leno)
    };
    /* A blocked lunge changes the body action without releasing the guard.
     * Its held-shoulder update comes before the hit and its attack cancels. */
    bfm_woods_fusion_sample_charge(run, !woods->action.blade_away &&
        (hit.guard_ready || woods->guarding), input.shoulder_held);
    hit.power = woods->incoming.power;
    hit.hit_flags = woods->incoming.flags;
    hit.attacker_yaw = woods->incoming.yaw;
    woods->incoming = (BfmWoodsCasterRingHit){0};
    if (!bfm_woods_player_hit_step(woods->receiver, run, &hit, &woods->reaction)) return 0;
    if (woods->reaction.hurt_started && hit.guard_ready)
        run->player.record[0x47] = 0; /* A damaging hit exits the original stance. */
    if (woods->reaction.guard_started) {
        woods->guarding = 1;
        woods->guard_pose = woods->action.pose;
        woods->guard_parts = woods->action.visible;
    }
    room_sound(woods, woods->reaction.sound);
    return 1;
}

static int water_events(BfmSouthernWoods *woods, BfmGameplayRun *run, const BfmVillagePlayer *player)
{
    if (player->events & BFM_VILLAGE_WATER_HIT) {
        memcpy(woods->water_contact, player->root.xyz, sizeof woods->water_contact);
        woods->in_water = 1;
        woods->guarding = 0;
    }
    if (player->events & BFM_VILLAGE_WATER_SPLASH) {
        if (run) {
            uint32_t flags = run->player.record[0x49] == 0x10 && u32le(run->player.record + 0x50)
                ? UINT32_C(0x800) : 0;
            if (!bfm_woods_player_hit_water_entry(woods->receiver, run, woods->water_contact,
                                                  flags, &woods->water_digits) ||
                !bfm_forest_water_effect_begin(&woods->splash, woods->water_contact,
                                               woods->presented_camera.eye, run)) return 0;
        }
        room_sound(woods, BFM_WOODS_WATER_ENTRY_SOUND);
    }
    if (player->events & BFM_VILLAGE_WATER_RETURN) {
        if (run) {
            BfmWoodsWaterLanding landing = bfm_woods_player_hit_water_return(woods->receiver, run);
            if (landing == BFM_WOODS_WATER_NO_LANDING) return 0;
            if (landing == BFM_WOODS_WATER_ZERO_HP) woods->life = CHECK_STOCK;
        }
        woods->in_water = 0;
    }
    return 1;
}

static int reaction_motion(BfmSouthernWoods *woods, BfmGameplayRun *run)
{
    BfmWoodsPlayerHitFrame *reaction = &woods->reaction;
    BfmWoodsReactionMotion motion = BFM_WOODS_HURT_HOLD;
    if (woods->guarding) motion = BFM_WOODS_GUARD_RECOIL;
    else if (reaction->hurt_started) motion = BFM_WOODS_HURT_START;
    BfmWoodsMotionContact ground;
    if (!bfm_village_motion_woods_reaction(woods->motion, woods->lilies, motion,
                                           reaction->recoil_x, reaction->recoil_z, &ground)) return 0;
    BfmWoodsPlayerHitContact contact = BFM_WOODS_HIT_SAME_SUPPORT;
    if (bfm_village_motion_player(woods->motion)->events & BFM_VILLAGE_WATER_HIT)
        contact = BFM_WOODS_HIT_SURFACE_REACTION;
    else if (ground.lost_support) contact = BFM_WOODS_HIT_LOST_SUPPORT;
    else if (ground.slope & 0x4000) contact = BFM_WOODS_HIT_SURFACE_REACTION;
    else if (ground.landed) contact = BFM_WOODS_HIT_LANDED;
    if (woods->guarding) bfm_woods_player_hit_guard_contact(woods->receiver, contact, reaction);
    else bfm_woods_player_hit_hurt_contact(woods->receiver, run, 0, contact, reaction);
    if (reaction->zero_hp) woods->life = CHECK_STOCK;
    if (reaction->recovered) {
        woods->guarding = 0;
        if (!reaction->guard_cancel && !reaction->surface_reaction && !reaction->solid_actor_reaction)
            return bfm_village_motion_woods_recover(woods->motion, reaction->resume_fall);
    }
    return 1;
}

/* Use the submitted player pose for field attachments, including when the
 * room advances without drawing or a recoil holds a different motion key. */
static int current_pose(BfmSouthernWoods *woods, const uint8_t flags[64], BfmBedroomPose *displayed)
{
    *displayed = bfm_village_motion_player(woods->motion)->pose;
    if (!bfm_woods_fusion_art_pose(woods->main, woods->main_bytes, woods->action.pose,
                                   woods->action.visible, woods->action.blade_away, displayed)) return 0;
    /* On a normal spell return the shared channel has already supplied this
     * field's face. The newly selected standing face advances next field. */
    if (!woods->spell.finished) bfm_woods_shrink_cast_pose(woods->shrink, displayed);
    if (woods->swing.action == 19) displayed->lumina_scale_q12 = 6635;
    if (woods->swing.pose) {
        if (!bfm_woods_fusion_art_pose(woods->main, woods->main_bytes, woods->swing.pose,
                                       (uint32_t)woods->swing.show_fusion << 22, 0, displayed))
            return 0;
        displayed->show_lumina = woods->swing.show_lumina;
    }
    uint32_t parts = bfm_village_field_visible(displayed, flags);
    uint32_t hurt_pose = woods->guarding ? woods->guard_pose : woods->reaction.pose;
    if (hurt_pose && !bfm_woods_fusion_art_pose(woods->main, woods->main_bytes, hurt_pose,
            woods->guarding ? woods->guard_parts : parts, woods->action.blade_away, displayed)) return 0;
    return 1;
}

static void visit_player_effect(void *context, unsigned slot, uint8_t kind)
{
    BfmSouthernWoods *woods = context;
    if (kind == BFM_WOODS_FOOT_DUST)
        bfm_woods_foot_dust_visit(&woods->foot, &woods->charge, slot);
}

static int step_charge(BfmSouthernWoods *woods, BfmGameplayRun *run, int entered, unsigned tick)
{
    if (entered) bfm_steam_charge_enter(&woods->charge, run->player.record[0x48]);
    BfmWoodsChargeArt *art = &woods->charge_art;
    BfmForestTransform *attachment = NULL;
    for (unsigned i = 0; i < BFM_STEAM_CHARGE_SLOTS; ++i) {
        const BfmSteamChargeSlot *slot = &woods->charge.slots[i];
        if (slot->kind != BFM_STEAM_CHARGE_EMITTER || slot->sound_only) continue;
        const BfmVillagePlayer *p = bfm_village_motion_player(woods->motion);
        BfmBedroomPose body;
        int animation[3];
        bfm_woods_satiate_effect_scale(woods->satiate, animation);
        unsigned scale = woods->reaction.miniature ? BFM_HERO_MINI_SCALE : BFM_HERO_NORMAL_SCALE;
        if (!current_pose(woods, woods->save->flags, &body) || !body.pose ||
            !bfm_bedroom_figure_attachment_root(&p->root, body.pitch, scale, animation, &art->model))
            return 0;
        memcpy(art->anchors, body.pose + 8, sizeof art->anchors);
        attachment = &art->model;
        break;
    }
    art->tick = tick;
    art->gauge = run->player.record[0x47];
    if (!bfm_steam_charge_step_shared(&woods->charge, run, art->gauge,
                                      attachment, attachment ? art->anchors : NULL,
                                      visit_player_effect, woods)) return 0;
    if (woods->charge.play_charge) room_sound(woods, BFM_WOODS_CHARGE_SOUND);
    if (woods->charge.play_full) room_sound(woods, BFM_WOODS_FULL_CHARGE_SOUND);
    if (woods->charge.stop_charge) room_sound(woods, BFM_WOODS_STOP_CHARGE_SOUND);
    return 1;
}

/* The scene's walk target is local to the newly released Soldier. Both
 * orders and the conversation camera sample his placed model on this field. */
static int begin_rescue_approach(BfmSouthernWoods *woods, const BfmWoodsLardwickEvents *event)
{
    const BfmWoodsLardwickField *f = &woods->lardwick;
    const uint32_t scene_origin = UINT32_C(0x80128158);
    if (event->camera < scene_origin || woods->bytes < 16 ||
        event->camera - scene_origin > woods->bytes - 16) return 0;
    uint8_t station[8];
    for (unsigned i = 0; i < 4; ++i) {
        uint16_t v = i == 3 ? (uint16_t)f->soldier_yaw : (uint16_t)f->root.xyz[i];
        station[2*i] = (uint8_t)v;
        station[2*i+1] = (uint8_t)(v >> 8);
    }
    BfmForestTransform model;
    bfm_forest_actor_root(f->soldier_yaw, f->root.xyz, &model);
    uint32_t walk = (uint16_t)event->approach_x | ((uint32_t)(uint16_t)event->approach_z << 16);
    return bfm_village_talk_camera_frame(&woods->rescue_camera, station,
               woods->scene + (event->camera - scene_origin), &woods->presented_camera) &&
           bfm_woods_lardwick_player_queue(&woods->rescue_player, 4, walk, &model, f->root.xyz) &&
           bfm_woods_lardwick_player_queue(&woods->rescue_player, 10, 0, &model, f->root.xyz);
}

static int step_rescue_player(BfmSouthernWoods *woods, int yaw, int player_ready,
                              BfmForestMessage **opened)
{
    if (woods->rescue_awarding) {
        BfmLardwickAwardFrame frame;
        const BfmBedroomView *view = bfm_bedroom_field_view(woods->field.field);
        if (!bfm_woods_lardwick_award_step(&woods->lardwick.award, woods->motion, view,
                                           !woods->lardwick.window, &frame)) return 0;
        if (frame.open_notice &&
            !open_field_message(opened, bfm_woods_lardwick_notice(&woods->lardwick))) return 0;
        room_sound(woods, frame.sound);
        woods->rescue_player.ready = frame.complete;
        if (frame.complete) woods->rescue_awarding = 0;
        return 1;
    }
    return bfm_woods_lardwick_player_step(&woods->rescue_player, woods->motion, yaw, player_ready);
}

static int step_rescue_script(BfmSouthernWoods *woods, BfmGameplayRun *run,
                              BfmForestMessage **opened)
{
    BfmWoodsLardwickEvents event;
    if (!bfm_woods_lardwick_release_step(&woods->lardwick, run, woods->save, woods->main,
            woods->main_bytes, woods->presented_camera.eye, woods->rescue_player.ready,
            !woods->rescue_camera.phase, opened, &event)) return 0;
    room_sound(woods, event.sound);
    if (event.begin_approach && !begin_rescue_approach(woods, &event)) return 0;
    if (event.camera_release) bfm_village_talk_camera_release(&woods->rescue_camera);
    if (event.award_begin) {
        if (!bfm_woods_lardwick_award_begin(&woods->lardwick.award, woods->motion)) return 0;
        woods->rescue_awarding = 1;
        woods->rescue_player.ready = 0;
    }
    return 1;
}

int bfm_southern_woods_step_field_at(BfmSouthernWoods *woods, BfmGameplayRun *run,
                                     BfmFieldInput field, unsigned actor_tick, BfmForestMessage **opened)
{
    if (opened) *opened = NULL;
    woods->sound = 0;
    woods->command_count = 0;
    if (woods->life == CHECK_STOCK && run) {
        /* The ordinary ring and body reactions leave field inventory enabled. */
        unsigned slot;
        BfmWoodsReviveResult result = bfm_woods_revive_prepare(run, woods->save, 0, woods->main,
                                                               woods->main_bytes, &slot, opened);
        if (result == BFM_WOODS_REVIVE_ERROR) return 0;
        if (result == BFM_WOODS_REVIVE_UNAVAILABLE) {
            woods->life = DEFEAT_PENDING;
            return 1;
        }
        if (!bfm_village_motion_woods_revive(woods->motion, bfm_field_tired_stance(run->player.record))) {
            bfm_forest_message_free(*opened);
            *opened = NULL;
            return 0;
        }
        bfm_woods_revive_restore(run, woods->save, slot);
        woods->life = ALIVE;
        woods->guarding = 0;
        woods->last_action = bfm_village_motion_grounded(woods->motion) ? 0u : 3u;
        /* Dispatch the resumed action this field with the old pad sample
         * cleared. The next update samples fresh controls during the notice. */
        field = (BfmFieldInput){0};
    }
    if (woods->life != ALIVE) return 1;
    int rescuing = bfm_woods_lardwick_busy(&woods->lardwick);
    int rescue_locked = bfm_woods_lardwick_controls_locked(&woods->lardwick);
    if (rescue_locked) field = (BfmFieldInput){0};
    if (run) bfm_village_motion_woods_condition(woods->motion, bfm_field_tired_stance(run->player.record));
    const BfmVillagePlayer *p = bfm_village_motion_player(woods->motion);
    int actors_running = !bfm_woods_fusion_awarding(woods->fusion);
    int dog_talking = bfm_woods_leno_in_dialogue(woods->leno);
    int was_in_water = woods->in_water;
    /* Possessing the Water Scroll alone does not protect the player. */
    bfm_village_motion_water_bypass(woods->motion, woods->save->variables[0xcf] || !actors_running);
    if (actors_running) {
        bfm_forest_damage_digits_advance(&woods->water_digits);
        bfm_forest_water_effect_tick(&woods->splash);
    }
    bfm_woods_body_notice_step(&woods->body_notice);
    /* Ordinary outdoor time continues during the throw and absorption;
     * the acquisition pose and its three message pages suspend it. */
    int return_clock = (woods->save->destination[2] == 1 || woods->save->destination[2] == 3) &&
                       bfm_village_motion_ingress_clock_running(woods->motion);
    if (run && (p->controlled || was_in_water || return_clock) && actors_running && !dog_talking && !rescue_locked &&
        !bfm_village_motion_woods_departing(woods->motion))
        bfm_village_clock_step(woods->save, 0);
    bfm_village_daylight_step(&woods->daylight, woods->save->progress[0x37], woods->tiles);
    int16_t previous[3], endpoint[3];
    memcpy(previous,p->root.xyz,sizeof previous);
    BfmWoodsKingEvent king = {0};
    if (actors_running && !bfm_woods_king_prepare(woods->king, woods->save->flags, &king))
        return 0;
    if (king.body_reward) {
        unsigned delta = bfm_woods_king_reward_apply(&woods->king_reward, king.body_points, woods->save);
        if (delta) {
            bfm_woods_body_notice_start(&woods->body_notice, delta);
            room_sound(woods, BFM_BODY_LEVEL_SOUND);
        }
        if (run) {
            memcpy(run->banks.figure_prices, woods->save->figure_prices, sizeof run->banks.figure_prices);
            memcpy(run->banks.variables, woods->save->variables, sizeof run->banks.variables);
        }
    }
    if (run) {
        memcpy(run->player.record, woods->save->progress, sizeof run->player.record);
        if (!receive_contact(woods, run, field, !rescue_locked && actors_running && !was_in_water && !dog_talking && !woods->award_open))
            return 0;
    }
    int reacting = !was_in_water && woods->reaction.movement_locked;
    int begin_cut = !was_in_water && woods->reaction.guard_cancel == BFM_WOODS_HIT_CHARGED_ATTACK &&
                    !bfm_woods_leno_held(woods->leno);
    int cut_field = begin_cut || (woods->swing.action == 19 && !was_in_water && !reacting && !dog_talking &&
                                  actors_running && !woods->award_open && p->controlled);
    int sliding = bfm_village_motion_woods_sliding(woods->motion);
    int lift_busy = bfm_woods_carry_active(&woods->carry);
    int mini = woods->reaction.miniature;
    int may_read = !cut_field && !reacting && !sliding && !mini && !dog_talking && !lift_busy && field.square_pressed && !field.shoulder_held && !woods->action.blade_away;
    BfmWoodsShrinkInput spell = {
        .can_begin = p->controlled && !cut_field && !reacting && !sliding && !mini && !dog_talking && !lift_busy && actors_running && !woods->action.movement_locked &&
                     !woods->action.action && !woods->action.blade_away &&
                     !(may_read && in_sign_reach(woods,previous)) &&
                     bfm_village_motion_grounded(woods->motion),
        .circle_pressed = field.circle_pressed, .field_tick = actor_tick, .interrupted = reacting
    };
    int casting = run && bfm_woods_shrink_cast_claims(woods->shrink, &run->player, &spell);
    int locked = was_in_water || cut_field || reacting || casting || dog_talking || woods->action.movement_locked;
    int ground_action = !locked && !sliding && actors_running && !woods->award_open &&
                        bfm_village_motion_grounded(woods->motion) && !mini;
    int can_brace = ground_action && !woods->action.blade_away && !field.shoulder_held;
    BfmWoodsCarryFrame carry = {0};
    int carrying = 0;
    if (woods->reaction.release_carry) carry.event = bfm_woods_carry_interrupt(&woods->carry);
    else if (!was_in_water && !reacting && !cut_field) carrying = carry_action(woods, run, field, can_brace, &carry);
    if (carrying < 0) return 0;
    if (carry.started_throw) room_sound(woods, BFM_WOODS_CARRY_THROW_SOUND);
    if (run) {
        BfmWoodsPlayerGuardCancel cancel = woods->reaction.guard_cancel;
        int counter = !bfm_woods_leno_held(woods->leno) &&
            (cancel == BFM_WOODS_HIT_FUSION_ATTACK || cancel == BFM_WOODS_HIT_LUMINA_ATTACK);
        /* A recoil cancel has already chosen the weapon. R1 still held after
         * that field does not cancel the weapon's own attack program. */
        int continuing = woods->guard_swing && ground_action && !carrying &&
                         (woods->swing.action == 6 || !woods->action.blade_away);
        int supported = 1;
        if (cut_field) {
            woods->guarding = 0;
            if (!bfm_village_motion_woods_cut(woods->motion, woods->lilies, woods->reaction.cancel_countdown,
                    bfm_woods_caster_melee_cut_finishing(woods->melee), &supported)) return 0;
        }
        BfmWoodsCasterMeleeInput attack = {
            .charged_counter=begin_cut, .cut_support_changed=!supported,
            .permitted=cut_field || counter || continuing ||
                       (can_brace && !carrying && !woods->action.action &&
                        !(may_read && in_sign_reach(woods, previous))),
            .square_edge=counter ? cancel == BFM_WOODS_HIT_FUSION_ATTACK : field.square_pressed,
            .triangle_edge=counter ? cancel == BFM_WOODS_HIT_LUMINA_ATTACK : field.triangle_pressed,
            .moving=field.x || field.y, .jump_edge=field.cross_pressed,
            .actor_flags=(mini ? 0x400u : 0u) | (woods->action.blade_away ? 4u : 0u)
        };
        if (!bfm_woods_caster_melee_step(woods->melee, run, &p->root, &attack, &woods->swing))
            return 0;
        woods->guard_swing = (counter || continuing) && woods->swing.action;
        if (woods->swing.action) locked = 1;
        room_sound(woods, woods->swing.sound);
    }
    int yaw = bfm_village_motion_view_yaw(woods->presented_camera.eye,woods->presented_camera.target);
    int scripted_motion = rescuing && (woods->rescue_player.count || woods->rescue_awarding);
    if (scripted_motion) {
        if (!step_rescue_player(woods, yaw, !woods->swing.action, opened)) return 0;
    } else if (bfm_village_motion_transfer(woods->motion) && bfm_village_motion_woods_departing(woods->motion)) {
        if (!bfm_village_motion_woods_departure_step(woods->motion,yaw,run)) return 0;
    } else if (cut_field) {
        if (woods->swing.face_slot &&
            !bfm_village_motion_combat_face(woods->motion, woods->swing.face_slot)) return 0;
    } else if (reacting) {
        if (!reaction_motion(woods, run)) return 0;
    } else if (dog_talking) {
        const BfmWoodsLenoView *dog = bfm_woods_leno_view(woods->leno);
        if (!bfm_village_motion_leno_greeting(woods->motion, woods->lilies, actors_running,
                                              dog->xyz, bfm_woods_leno_speaking(woods->leno))) return 0;
    } else if (carrying) {
        if (!bfm_village_motion_carry(woods->motion, field.x, field.y, yaw,
                                     actors_running, woods->lilies, &carry)) return 0;
        if (!bfm_village_motion_grounded(woods->motion))
            carry.event = bfm_woods_carry_interrupt(&woods->carry);
    } else {
        int standing = !locked && !mini && !sliding && !woods->award_open && !woods->swing.action &&
                       !woods->action.action && !field.shoulder_held &&
                       !(may_read && in_sign_reach(woods, previous));
        if (!bfm_village_motion_step_woods_field(woods->motion, locked ? 0 : field.x, locked ? 0 : field.y,
                yaw, !locked && field.cross_pressed, !locked && field.cross_held,
                actors_running, woods->lilies, run, standing)) return 0;
    }
    if (rescuing && !scripted_motion) woods->rescue_player.ready = !woods->swing.action;
    memcpy(endpoint,p->root.xyz,sizeof endpoint);
    if (!water_events(woods, run, p)) return 0;
    if ((p->events & BFM_VILLAGE_WATER_HIT) && bfm_woods_carry_active(&woods->carry))
        carry.event = bfm_woods_carry_interrupt(&woods->carry);
    if (p->controlled && !was_in_water && woods->life == ALIVE) {
        for (unsigned pass = 0; pass < 2; ++pass) {
            int sign_hit = bfm_woods_sign_model_slide(woods->sign, previous, endpoint);
            int king_hit = bfm_woods_king_slide(woods->king, previous, endpoint, woods->age);
            if (!sign_hit && !king_hit) break;
        }
        int touching = !carrying && can_brace && (field.x || field.y) &&
                       dog_contact(woods, previous, endpoint);
        if (!reacting && !carrying && !bfm_woods_carry_brace(&woods->carry, touching, &carry))
            return 0;
        if (touching && !bfm_village_motion_brace(woods->motion, &carry)) return 0;
        if (memcmp(endpoint,p->root.xyz,sizeof endpoint) &&
            !bfm_village_motion_correct_contact(woods->motion,endpoint)) return 0;
    }
    if (actors_running) {
        bfm_woods_king_target(woods->king, p->root.xyz, &king);
        room_sound(woods, king.sound);
    }
    if (woods->life == CHECK_STOCK) {
        memcpy(woods->save->progress, run->player.record, sizeof woods->save->progress);
        return 1;
    }
    if (actors_running && !was_in_water && p->controlled) bfm_woods_king_touch(woods->king,p->root.xyz);
    if (opened && may_read && !locked && !carrying && p->controlled && in_sign_reach(woods,p->root.xyz)) {
        const int16_t *point = bfm_woods_sign_model_position(woods->sign);
        if (bfm_village_motion_begin_read(woods->motion,point) &&
            !open_field_message(opened, bfm_field_message_borrow(woods->sign_text, woods->sign_bytes)))
            return 0;
    }
    if (rescuing && !step_rescue_script(woods, run, opened)) return 0;
    if (actors_running) {
        bfm_southern_woods_rescue_field_step(woods, run, actor_tick);
        if (!update_dog(woods, run, &carry, opened)) return 0;
    }
    if (dog_talking && !bfm_woods_leno_in_dialogue(woods->leno) &&
        !bfm_village_motion_end_leno_greeting(woods->motion)) return 0;
    follow(woods,0);
    BfmVillageCamera *presented = &woods->presented_camera;
    if (bfm_woods_leno_camera_step(woods->leno, &woods->camera, presented)) {
        memcpy(woods->camera.eye, presented->eye, sizeof presented->eye);
        memcpy(woods->camera.target, presented->target, sizeof presented->target);
    }
    if (woods->rescue_camera.phase) {
        bfm_village_talk_camera_step(&woods->rescue_camera, presented);
        if (!woods->rescue_camera.phase) woods->camera = *presented;
    }
    if (!bfm_bedroom_field_set_focal(woods->field.field, presented->focal)) return 0;
    bfm_bedroom_field_viewpoint(woods->field.field, presented->eye, presented->target, 1, 1);
    const BfmBedroomView *view = bfm_bedroom_field_view(woods->field.field);
    if (actors_running && !step_charged_effects(woods, p)) return 0;
    if (run && actors_running) {
        if (woods->swing.rays && woods->swing.event != 11) sword_contacts(woods, p->root.yaw);
        if (!charged_contacts(woods, run, p->root.yaw, view) ||
            !bfm_woods_charged_impact_step(&woods->charged_impact, run, view)) return 0;
        if (!rescuing && bfm_woods_lardwick_controls_locked(&woods->lardwick)) {
            /* The hit was delivered this field. Acquiring the scripted player
             * exits either Lumina attack immediately, before its recovery. */
            BfmWoodsCasterMeleeFrame ended;
            if (!bfm_woods_caster_melee_step(woods->melee, run, &p->root,
                    &(BfmWoodsCasterMeleeInput){0}, &ended) ||
                !bfm_village_motion_combat_face(woods->motion, 17)) return 0;
            woods->swing.pose = woods->swing.selected_pose = 0;
            woods->swing.action = woods->guard_swing = 0;
            field = (BfmFieldInput){0};
        }
    }
    if (actors_running) {
        if (!bfm_woods_caster_step(woods->caster, view, &p->root, run)) return 0;
        if (!woods->in_water) queue_enemy_contact(woods, &king);
    }
    if (run) {
        unsigned pickup = 0;
        if (actors_running && !bfm_woods_caster_burst_tick(woods->burst, run)) return 0;
        if (actors_running) {
            room_sound(woods, bfm_woods_caster_west_sound(woods->caster));
            room_sound(woods, bfm_woods_caster_cast_sound(woods->caster));
        }
        if (actors_running && !bfm_woods_caster_loot_step(woods->loot, run, p->root.xyz,
                p->controlled && !locked && !carrying, &pickup)) return 0;
        room_sound(woods, pickup);
        if (actors_running && !caster_rewards(woods, run)) return 0;
        memcpy(run->banks.variables, woods->save->variables, sizeof run->banks.variables);
        int guard_return = reacting && woods->reaction.recovered && !woods->reaction.guard_cancel &&
                           !woods->reaction.resume_fall && !woods->reaction.surface_reaction &&
                           !woods->reaction.solid_actor_reaction && !woods->action.blade_away &&
                           woods->last_action == 27 && field.shoulder_held;
        BfmWoodsFusionInput cast = {
            .actor_tick=actor_tick, .charge_sampled=1,
            .body_interrupted=(reacting && !guard_return) || woods->in_water,
            .controllable=!was_in_water && p->controlled && (!reacting || guard_return) && !sliding && !cut_field && !mini && !woods->swing.action && !woods->award_open && !casting && !carrying && !dog_talking &&
                          bfm_village_motion_grounded(woods->motion),
            .moving=p->root.xyz[0] != previous[0] || p->root.xyz[2] != previous[2],
            .shoulder=field.shoulder_held, .square_edge=!reacting && field.square_pressed,
            .visible=bfm_village_field_visible(&p->pose, woods->save->flags), .eye=view->eye
        };
        if (!bfm_woods_fusion_step(woods->fusion, woods->caster, run, &run->player, &p->root,
                                   woods->age, &cast, &woods->action)) return 0;
        if (woods->in_water) spell.interrupted = 1;
        spell.eye = view->eye;
        spell.target_root = bfm_woods_king_shrink_root(woods->king);
        if (!bfm_woods_shrink_cast_step(woods->shrink, &run->player, &p->root, &spell,
                                       &woods->spell)) return 0;
        if ((woods->action.award_face_slot || woods->action.award_finished) &&
            !bfm_village_motion_ability_pose(woods->motion, woods->action.award_face_slot,
                                            woods->action.award_finished)) return 0;
        if (woods->spell.face_slot && !bfm_village_motion_ability_pose(woods->motion,
                woods->spell.face_slot, woods->spell.finished)) return 0;
        if (woods->spell.target_contact && !bfm_woods_king_shrink(woods->king)) return 0;
        room_sound(woods, woods->spell.sound);
        if (woods->action.release_award) room_sound(woods, BFM_WOODS_SHRINK_SOUND);
        BfmWoodsSatiateEvent passive = BFM_WOODS_SATIATE_QUIET;
        if (actors_running && p->controlled && !dog_talking && !woods->award_open)
            passive = bfm_woods_satiate_step(woods->fusion, &run->player,
                                             reacting ? 25u : (unsigned)woods->action.action);
        if (actors_running) {
            unsigned sound = bfm_woods_satiate_effect_tick(woods->satiate, run, p->root.xyz, view->eye,
                    actor_tick, run->player.record[0x49] & 127, passive == BFM_WOODS_SATIATE_BEGIN,
                    woods->tiles);
            room_sound(woods, sound);
            bfm_woods_mini_smoke_step(&woods->mini_smoke, woods->reaction.applied_mini,
                                      p->root.xyz, view->eye);
        }
        /* A guarded hit can resume the same held stance. Only a fresh
         * ordinary entry creates a new charge emitter. */
        int entered_charge = woods->action.action == 17 && woods->last_action != 17 && !guard_return;
        if (actors_running) {
            /* A higher-priority attack or stance consumes the input before the
             * ordinary run/stop program can emit a puff that field. */
            if (!locked && !carrying && !woods->action.action && !woods->last_action &&
                !woods->action.open_award && !bfm_woods_carry_active(&woods->carry)) {
                BfmForestDustEmission event = bfm_village_motion_foot_dust(woods->motion, actor_tick);
                bfm_woods_foot_dust_emit(&woods->foot, &woods->charge, event,
                                         p->root.xyz, view->eye, p->root.yaw);
            }
            if (!step_charge(woods, run, entered_charge, actor_tick)) return 0;
        }
        if (reacting && !woods->reaction.recovered)
            woods->last_action = woods->guarding ? 27u : 25u;
        else if (bfm_village_motion_woods_sliding(woods->motion)) woods->last_action = 4;
        else if (cut_field && !bfm_village_motion_grounded(woods->motion)) woods->last_action = 3;
        else if (bfm_woods_carry_active(&woods->carry)) woods->last_action = 13;
        else woods->last_action = woods->swing.action ? woods->swing.action : (unsigned)woods->action.action;
        memcpy(woods->save->progress, run->player.record, sizeof woods->save->progress);
        if (woods->action.open_award) {
            BfmForestMessage *page = bfm_woods_ability_message_open(woods->scene, woods->bytes,
                woods->main, woods->main_bytes, woods->action.award_ability);
            if (!open_field_message(opened, page)) return 0;
            woods->award_open = 1;
        }
    }
    unsigned released = run && woods->action.release_award ? woods->action.award_ability : 0;
    if (!bfm_woods_award_effect_step_ability(&woods->apparition, woods->disc, woods->scene,
                                             woods->bytes, woods->tiles, released)) return 0;
    ++woods->age;
    return bfm_village_motion_face(woods->motion,woods->tiles) &&
           (woods->spell.finished || bfm_woods_shrink_cast_face(woods->shrink,woods->tiles));
}

void bfm_southern_woods_rescue_field_step(BfmSouthernWoods *woods, BfmGameplayRun *run,
                                          unsigned field_tick)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(woods->motion);
    bfm_woods_lardwick_field_step(&woods->lardwick, run, player->root.xyz, field_tick);
}

int bfm_southern_woods_detector(const BfmSouthernWoods *woods)
{
    return woods && woods->lardwick.active ? (int)woods->lardwick.detector_step : -1;
}

int bfm_southern_woods_step_field(BfmSouthernWoods *woods, BfmGameplayRun *run,
                                  BfmFieldInput field, BfmForestMessage **opened)
{
    return bfm_southern_woods_step_field_at(woods, run, field, woods->age, opened);
}

int bfm_southern_woods_step(BfmSouthernWoods *woods, int x, int y,
                            int talk_edge, BfmForestMessage **opened)
{
    return bfm_southern_woods_step_field(woods, NULL,
        (BfmFieldInput){.x=x, .y=y, .square_pressed=talk_edge}, opened);
}

static void draw_actors(void *context, int depth)
{
    BfmSouthernWoods *woods = context;
    for (;;) {
        int sign = bfm_woods_sign_model_next_depth(woods->sign);
        int hero = bfm_bedroom_figure_next_depth(woods->field.hero);
        int next = sign > hero ? sign : hero;
        if (next == INT_MIN || next < depth) break;
        bfm_woods_sign_model_draw_to(woods->sign,next);
        bfm_bedroom_figure_draw_to(woods->field.hero,next);
    }
    if (depth == INT_MIN) bfm_bedroom_figure_draw_to(woods->field.hero,depth);
}

int bfm_southern_woods_draw(BfmSouthernWoods *woods, const uint8_t flags[64],
                            const BfmForestMessageFrame *window, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_southern_woods_player(woods);
    const size_t room = 0x8018ea38u - 0x80128158u;
    BfmBedroomPose displayed;
    if (!current_pose(woods, flags, &displayed)) return 0;
    uint32_t parts = woods->retry || woods->reaction.hide_body || p->water_hidden ? 0 :
                     bfm_village_field_visible(&displayed, flags);
    BfmBedroomRoot displayed_root = p->root;
    if (p->water_hidden) displayed_root.shadow_visible = 0;
    int animation[3];
    bfm_woods_satiate_effect_scale(woods->satiate, animation);
    if (!bfm_woods_caster_faces(woods->caster, woods->tiles)) return 0;
    if (!bfm_bedroom_figure_lights(woods->field.hero, woods->spell.player_lit ?
                                   woods->spell.lamps : woods->daylight.lamps) ||
        !bfm_village_sky_draw_lit(woods->scene+room,woods->bytes-room,woods->daylight.sky,pixels) ||
        !bfm_village_field_prepare_axes(&woods->field, &woods->presented_camera, &displayed_root, &displayed,
                                      flags, woods->age, parts, woods->reaction.miniature ? 1365 : 4096,
                                      animation, woods->tiles, &pixels[0][0])) return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(woods->field.field);
    if ((woods->spell.player_lit && !bfm_bedroom_figure_lights(woods->field.hero, woods->daylight.lamps)) ||
        !bfm_woods_lilies_add(woods->lilies, woods->field.hero, view) ||
        !bfm_woods_king_add(woods->king, woods->field.hero, view) ||
        (woods->lardwick_art && !bfm_woods_lardwick_art_add(woods->lardwick_art,
            woods->field.hero, view, &woods->lardwick, woods->age)) ||
        !bfm_woods_water_effect_append(&woods->splash, woods->field.hero, view) ||
        !bfm_woods_fusion_art_append(woods->field.hero, view, NULL, NULL, &woods->water_digits) ||
        !bfm_woods_caster_add(woods->caster, woods->field.hero, view) ||
        !bfm_woods_fusion_art_cut(woods->field.hero, view, woods->charged_fx) ||
        !bfm_woods_charged_impact_add(&woods->charged_impact, woods->field.hero, view) ||
        !bfm_woods_fusion_art_append(woods->field.hero, view, NULL, NULL, woods->reaction.digits) ||
        !bfm_woods_mini_smoke_add(&woods->mini_smoke, woods->field.hero, view) ||
        !bfm_woods_satiate_effect_add(woods->satiate, woods->field.hero, view) ||
        !bfm_woods_caster_burst_add(woods->burst, woods->field.hero, view) ||
        !bfm_woods_caster_loot_add(woods->loot, woods->field.hero, view, woods->age) ||
        !bfm_woods_leno_actor_add(woods->leno_art, woods->field.hero, view,
                                  bfm_woods_leno_view(woods->leno)) ||
        !bfm_woods_shrink_cast_append(woods->shrink, woods->field.hero, view) ||
        !bfm_woods_fusion_art_append(woods->field.hero, view, woods->action.blade,
                                     woods->action.trails, woods->action.bp_digits) ||
        !bfm_woods_charge_art_append(woods->field.hero, view, &woods->charge, &woods->foot,
                                     &woods->charge_art) ||
        !bfm_woods_fusion_art_lights(woods->field.hero, view, &woods->action) ||
        !bfm_woods_award_effect_append(&woods->apparition, woods->field.hero, view) ||
        !bfm_woods_sign_model_prepare(woods->sign,view,
                                      woods->tiles,&pixels[0][0])) return 0;
    bfm_woods_water_draw(&woods->water, woods->age, woods->field.field, woods->tiles,
                         &pixels[0][0], draw_actors, woods);
    if (window && woods->lardwick.window == BFM_LARDWICK_WINDOW_SOLDIER) {
        int speaker[3], hero[3];
        if (!bfm_bedroom_project(view, woods->lardwick.root.xyz, speaker) ||
            !bfm_bedroom_project(view, p->root.xyz, hero) ||
            !bfm_forest_message_art_draw_placed(woods->writing, window, (int16_t)(speaker[0]-160),
                                                speaker[0] >= hero[0], pixels)) return 0;
    } else if (window) {
        bfm_forest_message_art_draw(woods->writing, window, pixels);
    }
    bfm_woods_body_notice_draw(&woods->body_notice, woods->tiles, pixels);
    return 1;
}

/* SPDX-License-Identifier: MIT */
#include "hells_valley.h"
#include "hells_valley_boulders.h"
#include "hells_valley_view.h"
#include "bedroom_field.h"
#include "bedroom_effects.h"
#include "village_arrival.h"
#include "field_rest.h"
#include "forest_bank_thorns.h"
#include "valley_earth_cast.h"
#include "valley_bincho.h"
#include "valley_knitter.h"
#include "valley_gate_team.h"
#include "valley_gate_door.h"
#include "valley_gate_people.h"
#include "woods_lardwick_player.h"
#include "woods_twinpeak_player_blade.h"
#include "valley_defeat.h"
#include "valley_hinge_debris.h"
#include "valley_quake_art.h"
#include "valley_rock_shake.h"
#include "valley_scroll_menu.h"
#include "scene_tiles.h"
#include "village_clock.h"
#include "woods_fusion_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

struct BfmHellsValley {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    BfmHellsValleyView *view;
    BfmValleyDefeat *defeat;
    BfmHellsValleyBoulders *boulders;
    BfmValleyBincho *bincho;
    BfmValleyKnitter *knitter;
    BfmValleyGateTeam gate_team;
    BfmValleyGatePeople *gate_people;
    BfmValleyGateDoor *gate_door;
    BfmWoodsLardwickPlayer gate_turn;
    int gate_standing;
    BfmWoodsCasterMelee *melee;
    BfmWoodsCasterMeleeFrame swing;
    BfmWoodsFootDust foot;
    BfmFieldRest rest;
    BfmForestBankThornHurt thorn;
    BfmForestBankThornFrame thorn_frame;
    BfmForestDamageDigits damage_digits, credit_digits;
    const uint8_t *digit_palette;
    unsigned blink;
    int hidden;
    BfmValleyScrollMenu *menu;
    const MusashiDiscMedia *disc;
    const uint8_t *main;
    size_t main_bytes, scene_bytes;
    BfmValleyEarthCast earth;
    BfmValleyHingeDebris debris;
    BfmValleyQuakeArt waves;
    BfmValleyRockShake shake;
    BfmBedroomRoot cast_origin;
    int casting;
    BfmSteamChargeEffects effects;
    BfmVillageMotion *motion;
    BfmSaveRecord *save;
    uint16_t (*tiles)[1024];
    unsigned sounds[BFM_VILLAGE_WOODS_SOUNDS];
    size_t sound_count;
    unsigned age;
    int hud_initialized;
};

const BfmVillagePlayer *bfm_hells_valley_player(const BfmHellsValley *v)
{
    return v ? bfm_village_motion_player(v->motion) : NULL;
}

const BfmVillageMotion *bfm_hells_valley_departure(const BfmHellsValley *v)
{
    return v && bfm_village_motion_transfer(v->motion) ? v->motion : NULL;
}

void bfm_hells_valley_free(BfmHellsValley *v)
{
    if (!v) return;
    bfm_valley_scroll_menu_free(v->menu);
    bfm_woods_caster_melee_free(v->melee);
    bfm_valley_gate_people_free(v->gate_people);
    bfm_valley_gate_door_free(v->gate_door);
    bfm_valley_knitter_free(v->knitter);
    bfm_valley_bincho_free(v->bincho);
    bfm_valley_defeat_free(v->defeat);
    bfm_hells_valley_view_free(v->view);
    bfm_hells_valley_boulders_free(v->boulders);
    bfm_village_motion_free(v->motion);
    free(v);
}

BfmHellsValley *bfm_hells_valley_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    if (!disc || !entry || !main || !main_bytes || !tiles || entry->scene_member != 31 ||
        u32le(entry->state.destination) != 0x302bu) return NULL;
    BfmHellsValley *v = calloc(1, sizeof *v);
    if (!v) return NULL;
    size_t bytes = bfm_read_archive_field_scene(disc, "SC03.CD", 31, v->scene);
    const size_t palette = UINT32_C(0x801854cc) - UINT32_C(0x80128158);
    if (bytes < palette + 5 || !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", 31, tiles)) goto fail;
    v->digit_palette = v->scene + palette;
    v->motion = bfm_village_motion_valley(v->scene, bytes, main, main_bytes,
                                           entry->state.destination, disc);
    const BfmVillagePlayer *p = bfm_hells_valley_player(v);
    if (!p || !p->pose.pose) goto fail;
    v->view = bfm_hells_valley_view_open(disc, v->scene, bytes, entry->state.progress[0x37], tiles);
    v->boulders = bfm_hells_valley_boulders_open(v->scene, bytes, entry->state.flags);
    if (!v->view || !v->boulders || !bfm_valley_rock_shake_load(&v->shake, v->scene, bytes, main, main_bytes) ||
        !bfm_valley_debris_art_load(&v->debris.art, v->scene, bytes) ||
        !bfm_woods_foot_dust_load(&v->foot, disc) ||
        !bfm_village_motion_valley_orb(v->motion, entry->state.progress[0x48], tiles)) goto fail;
    v->melee = bfm_valley_melee_open(disc, v->scene, bytes, main, main_bytes);
    v->bincho = bfm_valley_bincho_open(v->scene, bytes, entry->state.variables);
    v->knitter = bfm_valley_knitter_open(v->scene, bytes, main, main_bytes, &entry->state);
    unsigned stage = u32le(entry->state.progress + 8);
    v->gate_people = bfm_valley_gate_people_open(v->scene, bytes, stage, tiles);
    v->gate_door = bfm_valley_gate_door_open(v->scene, bytes, stage);
    if (!v->melee || !v->bincho || !v->knitter || !v->gate_people || !v->gate_door ||
        !bfm_valley_gate_team_init(&v->gate_team, v->scene, bytes, &entry->state)) goto fail;
    bfm_village_motion_valley_obstacles(v->motion, v->bincho, v->gate_door);
    bfm_village_motion_valley_boulders(v->motion, v->boulders, 0);
    v->defeat = bfm_valley_defeat_open(disc, v->scene, bytes, main, main_bytes, tiles,
                                      bfm_hells_valley_view_field(v->view), &v->age);
    if (!v->defeat) goto fail;
    v->save = &entry->state;
    v->tiles = tiles;
    v->disc = disc;
    v->main = main;
    v->main_bytes = main_bytes;
    v->scene_bytes = bytes;
    bfm_hells_valley_view_follow(v->view, p->root.xyz, 1, 0, 0);
    return v;
fail:
    bfm_hells_valley_free(v);
    return NULL;
}

static void queue_sound(BfmHellsValley *v, unsigned command);

static int step_hero(BfmHellsValley *v, BfmGameplayRun *run, BfmFieldInput input, int yaw, int *quake)
{
    const BfmVillagePlayer *p = bfm_hells_valley_player(v);
    int grounded = p->controlled && bfm_village_motion_grounded(v->motion) &&
                   !bfm_village_motion_woods_sliding(v->motion) && !bfm_hells_valley_departure(v);
    if (grounded && !v->casting && !v->swing.action)
        bfm_field_rest_input(&v->rest, v->save, input);

    BfmValleyEarthFrame spell = bfm_valley_earth_cast_step(&v->earth, v->save, input,
        grounded && !v->swing.action && v->rest.pose == BFM_FIELD_REST_FREE);
    *quake = spell.quake;
    if (spell.casting) {
        if (!v->casting) v->cast_origin = p->root;
        v->casting = 1;
        BfmBedroomRoot root = v->cast_origin;
        root.xyz[1] = (int16_t)(root.xyz[1] + spell.height);
        int ended;
        return bfm_village_motion_staged_program(v->motion, &root, spell.program, &ended);
    }
    v->casting = 0;
    if (v->rest.pose != BFM_FIELD_REST_FREE) {
        int result = bfm_village_motion_field_rest(v->motion, v->rest.pose, v->rest.ticks);
        if (!result) return 0;
        if (result == 2 && v->rest.pose == BFM_FIELD_REST_WAKE) v->save->progress[0x47] = 0;
        bfm_field_rest_advance(&v->rest, result == 2);
        return 1;
    }
    if (run) {
        BfmWoodsCasterMeleeInput attack = {
            .permitted=grounded && !input.shoulder_held && !spell.finished,
            .square_edge=input.square_pressed, .triangle_edge=input.triangle_pressed,
            .jump_edge=input.cross_pressed, .moving=input.x || input.y,
            .actor_flags=(v->save->progress[0x23] & 0x80) ? 0x400u : 0u
        };
        if (!bfm_woods_caster_melee_step(v->melee, run, &p->root, &attack, &v->swing)) return 0;
        if (v->swing.sound) queue_sound(v, v->swing.sound);
        if (v->swing.action) input = (BfmFieldInput){0};
    }
    if (!bfm_village_motion_step_valley_field(v->motion, input.x, input.y, yaw,
                                              input.cross_pressed, input.cross_held, run)) return 0;
    if (spell.charging || spell.finished) {
        /* Direct programs select the mounted charging body. Re-enter ordinary
         * stance after the downward strike finishes, even without a pad edge. */
        p = bfm_hells_valley_player(v);
        if (spell.finished)
            return bfm_village_motion_staged_action(v->motion, &p->root, BFM_BEDROOM_STAND);
        int ended;
        return bfm_village_motion_staged_program(v->motion, &p->root, spell.program, &ended);
    }
    return 1;
}

static void queue_sound(BfmHellsValley *v, unsigned command)
{
    if (v->sound_count < BFM_VILLAGE_WOODS_SOUNDS) v->sounds[v->sound_count++] = command;
}

static int receive_thorns(BfmHellsValley *v, BfmGameplayRun *run, unsigned tick)
{
    const BfmVillagePlayer *p = bfm_hells_valley_player(v);
    uint16_t *reason = bfm_village_motion_bank_pending(v->motion);
    if (!reason) return 0;
    unsigned busy = !p->controlled || v->casting || bfm_hells_valley_departure(v) != NULL;
    BfmWoodsCasterRingHit incoming = {0};
    int struck = bfm_forest_bank_thorn_hit(reason, busy, v->save->progress[0x49],
                                          (uint16_t)p->root.yaw, &incoming);
    BfmForestBankThornInput input = {
        .incoming = struck ? &incoming : NULL, .disc = v->disc, .field = tick,
        .scene_state = busy, .player_action = v->thorn_frame.action
    };
    memcpy(run->player.record, v->save->progress, sizeof run->player.record);
    if (!bfm_village_motion_bank_thorn(v->motion, &v->thorn, run, input, &v->thorn_frame))
        return 0;
    memcpy(v->save->progress, run->player.record, sizeof run->player.record);
    bfm_forest_damage_digits_advance(&v->damage_digits);
    bfm_forest_damage_digits_advance(&v->credit_digits);
    v->hidden = v->blink ? (--v->blink & 1u) : 0;
    if (v->thorn_frame.movement_locked) v->hidden = v->thorn_frame.hide_body;
    if (v->thorn_frame.blink_fields) v->blink = v->thorn_frame.blink_fields;
    if (v->thorn_frame.started) {
        v->rest = (BfmFieldRest){0};
        v->earth = (BfmValleyEarthCast){0};
        queue_sound(v, v->thorn_frame.sound);
        const BfmForestBankThornFrame *frame = &v->thorn_frame;
        if (frame->damage_number) {
            bfm_forest_damage_digits_start_data(&v->damage_digits, v->digit_palette + 1,
                                                frame->effect_root, (uint16_t)frame->damage_number);
            v->damage_digits.hurt = 1;
            bfm_forest_damage_digits_scatter(&v->damage_digits, bfm_gameplay_random(run));
        }
        if (frame->bp_credit) {
            bfm_forest_damage_digits_start_data(&v->credit_digits, v->digit_palette + 4,
                                                frame->effect_root, (uint16_t)frame->bp_credit);
            v->credit_digits.bp_credit = 1;
        }
    }
    return 1;
}

static void boulder_effects(BfmHellsValley *v, const BfmValleyBoulderEvents *rocks)
{
    for (unsigned bit = 1; bit <= (rocks->started | rocks->shake | rocks->landed); bit <<= 1) {
        if (rocks->started & bit) queue_sound(v, BFM_VALLEY_BOULDER_START_SOUND);
        if (rocks->shake & bit) bfm_valley_rock_shake_request(&v->shake, BFM_VALLEY_SHAKE_ROLL);
        if (rocks->landed & bit) {
            bfm_valley_rock_shake_request(&v->shake, BFM_VALLEY_SHAKE_LANDING);
            queue_sound(v, BFM_VALLEY_BOULDER_IMPACT_SOUND);
        }
    }
    if (rocks->hinge.impact) queue_sound(v, BFM_VALLEY_HINGE_IMPACT_SOUND);
}

size_t bfm_hells_valley_take_sounds(BfmHellsValley *v, unsigned *out, size_t capacity)
{
    if (!v || !out || !capacity) return 0;
    size_t count = capacity < v->sound_count ? capacity : v->sound_count;
    memcpy(out, v->sounds, count * sizeof *out);
    v->sound_count -= count;
    memmove(v->sounds, v->sounds + count, v->sound_count * sizeof *out);
    return count;
}

static int step_bincho(BfmHellsValley *v, BfmGameplayRun *run, const BfmVillagePlayer *p,
                       unsigned actor_tick, int rescuing, BfmForestMessage **opened)
{
    if (!rescuing && v->swing.rays &&
        bfm_valley_bincho_hit(v->bincho, bfm_village_motion_ground(v->motion), &v->swing)) {
        queue_sound(v, BFM_WOODS_CRYSTAL_HIT_SOUND);
        bfm_woods_caster_melee_connected(v->melee);
        /* The blade remains extended behind the first crystal glint. Its
         * ordinary program finishes while the release owns new inputs. */
    }
    BfmValleyBinchoEvents crystal;
    const BfmVillageCamera *presented = bfm_hells_valley_view_submitted(v->view);
    if (!bfm_valley_bincho_step(v->bincho, run, p->root.xyz, presented->eye,
                                actor_tick, &crystal)) return 0;
    if (crystal.sound) queue_sound(v, crystal.sound);
    BfmValleyKnitterEvents rescue;
    if (!bfm_valley_knitter_step(v->knitter, run, v->motion, presented,
            bfm_hells_valley_projection(v->view), !v->swing.action,
            bfm_valley_bincho_root(v->bincho), crystal.captive_yaw, crystal.reveal, crystal.approach,
            opened, &rescue)) return 0;
    if (rescue.player_sound) queue_sound(v, rescue.player_sound);
    if (rescue.actor_sound) queue_sound(v, rescue.actor_sound);
    return 1;
}

static int step_gate(BfmHellsValley *v, BfmGameplayRun *run, const BfmVillagePlayer *p,
                     int player_ready, BfmForestMessage **opened)
{
    BfmValleyGateResult stage = bfm_valley_gate_team_step(&v->gate_team, p->root.xyz[2],
                        player_ready && v->gate_turn.ready, v->save, run, opened);
    if (stage == BFM_VALLEY_GATE_ERROR) return 0;
    if (v->gate_team.face_point) {
        int16_t goal[3] = {v->gate_team.face_x, p->root.xyz[1], v->gate_team.face_z};
        if (!bfm_woods_lardwick_player_queue(&v->gate_turn, 10, 0, NULL, goal)) return 0;
    }
    int landed = bfm_valley_gate_people_step(v->gate_people, v->gate_team.member);
    if (landed) queue_sound(v, BFM_VALLEY_SOLDIER_LAND_SOUND);
    BfmValleyGateDoorEvents door = bfm_valley_gate_door_step(v->gate_door, run, landed);
    if (door.shake) bfm_valley_rock_shake_request(&v->shake, BFM_VALLEY_SHAKE_GATE);
    for (unsigned leaf = 0; leaf < door.slide_sounds; ++leaf)
        queue_sound(v, BFM_VALLEY_GATE_SLIDE_SOUND);
    return 1;
}

int bfm_hells_valley_step_with_notice(BfmHellsValley *v, BfmGameplayRun *run, BfmFieldInput input,
                                    unsigned actor_tick, BfmForestMessage **opened)
{
    const BfmVillagePlayer *p = bfm_hells_valley_player(v);
    if (!p) return 0;
    if (bfm_valley_defeat_retry(v->defeat)) return 1;
    if (run && v->thorn_frame.zero_hp) {
        BfmValleyDefeatResult life = bfm_valley_defeat_resolve(v->defeat, v->motion, v->save, run,
                                          bfm_hells_valley_view_submitted(v->view), opened);
        if (life == BFM_VALLEY_DEFEAT_ERROR) return 0;
        v->thorn_frame.zero_hp = 0;
        if (life == BFM_VALLEY_RETRY) {
            v->hidden = 1;
            return 1;
        }
        input = (BfmFieldInput){0};
    }
    int rescuing = bfm_valley_bincho_releasing(v->bincho) || bfm_valley_knitter_busy(v->knitter);
    int at_gate = bfm_valley_gate_team_active(&v->gate_team);
    int scripted = rescuing || at_gate;
    int menu_allowed = p->controlled && !v->thorn.phase && !scripted && !v->swing.action;
    if (menu_allowed && input.start_pressed && !v->menu) {
        v->menu = bfm_valley_scroll_menu_open(v->disc, v->scene, v->scene_bytes,
                                              v->main, v->main_bytes, v->tiles);
        if (!v->menu) return 0;
    }
    if ((bfm_valley_scroll_menu_active(v->menu) || menu_allowed) &&
        bfm_valley_scroll_menu_input(v->menu, input.start_pressed, input, v->save)) {
        if (run) run->player.record[0x48] = v->save->progress[0x48];
        return 1;
    }
    if (run && !scripted && !bfm_hells_valley_departure(v) &&
        (p->controlled || bfm_village_motion_ingress_clock_running(v->motion))) {
        int sleeping = p->controlled && bfm_field_rest_sleeping(&v->rest);
        if (p->controlled) bfm_field_rest_stamina(v->save, sleeping);
        bfm_village_clock_step(v->save, sleeping);
    }
    const BfmVillageCamera *camera = bfm_hells_valley_view_camera(v->view);
    int quake = 0;
    bfm_village_motion_valley_boulders(v->motion, v->boulders, actor_tick);
    if (run && !receive_thorns(v, run, actor_tick)) return 0;
    int player_ready = bfm_village_motion_grounded(v->motion) && !v->thorn.phase &&
                       !v->swing.action && !v->casting && v->rest.pose == BFM_FIELD_REST_FREE;
    if (at_gate && player_ready) {
        if (v->gate_turn.count) {
            if (!bfm_woods_lardwick_player_step(&v->gate_turn, v->motion, camera->yaw, 1)) return 0;
        } else {
            BfmBedroomRoot root = p->root;
            root.locomotion_edge = BFM_TRAVEL_NONE;
            BfmBedroomAction pose = v->gate_standing ? BFM_BEDROOM_NONE : BFM_BEDROOM_STAND;
            if (!bfm_village_motion_staged_action(v->motion, &root, pose)) return 0;
            v->gate_standing = 1;
            v->gate_turn.ready = 1;
        }
    } else if (!v->thorn_frame.movement_locked && !bfm_valley_knitter_drives_player(v->knitter)) {
        BfmFieldInput hero_input = scripted ? (BfmFieldInput){0} : input;
        if (!step_hero(v, run, hero_input, (camera->yaw + 2048) & 4095, &quake)) return 0;
    }
    p = bfm_hells_valley_player(v);
    bfm_hells_valley_view_follow_captive(v->view, p->root.xyz, 0, scripted ? 0 : input.camera_pressed,
                                v->shake.world_y, v->knitter);
    if (run && !step_bincho(v, run, p, actor_tick, rescuing, opened)) return 0;
    BfmValleyBoulderEvents rocks = bfm_hells_valley_boulders_step(v->boulders, bfm_hells_valley_projection(v->view),
        bfm_village_motion_ground(v->motion), quake, p->root.xyz, v->save->flags, run ? run->story_flags : NULL);
    if (run && !step_gate(v, run, p, player_ready, opened)) return 0;
    if (quake) {
        queue_sound(v, BFM_VALLEY_EARTH_LANDING_SOUND);
        bfm_valley_rock_shake_request(&v->shake, BFM_VALLEY_SHAKE_EARTH_STRIKE);
    }
    if (bfm_valley_quake_art_step(&v->waves, quake, p->root.xyz, p->root.yaw))
        queue_sound(v, BFM_VALLEY_EARTH_RING_SOUND);
    boulder_effects(v, &rocks);
    bfm_valley_hinge_debris_step(&v->debris, run, rocks.hinge.impact ? rocks.hinge.debris_origin : NULL);
    if (run) memcpy(run->player.record, v->save->progress, sizeof run->player.record);
    BfmForestDustEmission dust = bfm_village_motion_foot_dust(v->motion, actor_tick);
    bfm_woods_foot_dust_emit(&v->foot, &v->effects, dust, p->root.xyz, camera->eye, p->root.yaw);
    for (unsigned slot = 0; slot < BFM_STEAM_CHARGE_SLOTS; ++slot)
        bfm_woods_foot_dust_visit(&v->foot, &v->effects, slot);
    bfm_valley_rock_shake_step(&v->shake);
    bfm_hells_valley_view_light(v->view, v->save->progress[0x37]);
    ++v->age;
    return 1;
}

int bfm_hells_valley_step_at(BfmHellsValley *v, BfmGameplayRun *run, BfmFieldInput input,
                            unsigned actor_tick)
{
    BfmForestMessage *ignored = NULL;
    int result = bfm_hells_valley_step_with_notice(v, run, input, actor_tick, &ignored);
    bfm_forest_message_free(ignored);
    return result;
}

int bfm_hells_valley_step(BfmHellsValley *v, BfmGameplayRun *run, BfmFieldInput input)
{
    return v && bfm_hells_valley_step_at(v, run, input, v->age);
}

int bfm_hells_valley_detector(const BfmHellsValley *v)
{
    return v ? bfm_valley_bincho_detector(v->bincho) : -1;
}

int bfm_hells_valley_message_closed(BfmHellsValley *v)
{
    if (!v) return 0;
    if (v->gate_team.window) return bfm_valley_gate_team_message_closed(&v->gate_team);
    return bfm_valley_knitter_close(v->knitter);
}

int bfm_hells_valley_hud(BfmHellsValley *v, BfmForestHud *hud)
{
    if (!v || !hud) return 0;
    if (!v->hud_initialized) {
        bfm_forest_hud_enter_hidden(hud);
        v->hud_initialized = 1;
    }
    const uint8_t *status = v->save->progress;
    unsigned palette;
    if (bfm_village_hud_step_blade(hud, status[0x48], &palette) &&
        !bfm_village_motion_valley_orb(v->motion, palette, v->tiles)) return 0;
    if (bfm_valley_scroll_menu_active(v->menu)) bfm_castle_hud_command(hud);
    else bfm_castle_hud_command_closed(hud);
    int rest = (status[0x48] & 0x80) != 0;
    bfm_village_hud_rest(hud, rest, status[0x47], bfm_field_rest_sleeping(&v->rest));
    if (!rest) bfm_woods_hud_gauges(hud, status[0x47], -1);
    return 1;
}

static int draw_actors(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    BfmHellsValley *v = context;
    BfmForestCamera camera;
    if (!bfm_valley_gate_door_draw(v->gate_door, figure, view) ||
        !bfm_valley_gate_people_draw(v->gate_people, figure, view) ||
        !bfm_valley_bincho_draw(v->bincho, figure, view) ||
        !bfm_valley_knitter_add(v->knitter, figure, view, v->age) ||
        !bfm_hells_valley_boulders_draw(v->boulders, figure, view) ||
        !bfm_valley_quake_art_draw(&v->waves, figure, view) ||
        !bfm_woods_fusion_art_append(figure, view, NULL, NULL, &v->damage_digits) ||
        !bfm_woods_fusion_art_append(figure, view, NULL, NULL, &v->credit_digits) ||
        !bfm_woods_effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_FOREST_FOOT_DUST_CARDS + BFM_VALLEY_HINGE_CARDS];
    unsigned count = bfm_woods_foot_dust_project(&v->foot, &v->effects, &camera, cards,
                                                BFM_FOREST_FOOT_DUST_CARDS);
    count += bfm_valley_hinge_debris_project(&v->debris, &camera, cards + count);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}

int bfm_hells_valley_draw(BfmHellsValley *v, const uint8_t flags[64], uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_hells_valley_player(v);
    if (!p || !flags || !pixels || !bfm_village_motion_face(v->motion, v->tiles)) return 0;
    unsigned scale = v->save->progress[0x23] & 0x80 ? BFM_HERO_MINI_SCALE : BFM_HERO_NORMAL_SCALE;
    BfmBedroomPose shown = p->pose;
    if (!bfm_twinpeak_player_blade_pose(&v->swing, v->main, v->main_bytes, &shown)) return 0;
    return bfm_hells_valley_view_draw(v->view, &p->root, &shown, flags, v->age, scale, v->hidden,
                                      pixels, draw_actors, v) &&
           (!bfm_valley_scroll_menu_active(v->menu) || bfm_valley_scroll_menu_draw(v->menu, v->tiles, pixels));
}

BfmRetryScreen *bfm_hells_valley_retry(const BfmHellsValley *v)
{
    return v ? bfm_valley_defeat_retry(v->defeat) : NULL;
}

int bfm_hells_valley_draw_retry(BfmHellsValley *v, const BfmForestHud *hud, uint32_t pixels[240][320])
{
    BfmRetryScreen *retry = bfm_hells_valley_retry(v);
    if (!retry) return 0;
    if (bfm_retry_screen_keep_world(retry) && !bfm_hells_valley_draw(v, v->save->flags, pixels)) return 0;
    return bfm_valley_defeat_draw_retry(v->defeat, hud, pixels);
}

int bfm_hells_valley_draw_notice(BfmHellsValley *v, const BfmForestMessageFrame *notice,
                                uint32_t pixels[240][320])
{
    BfmForestMessageArt *writing = bfm_valley_defeat_window_art(v->defeat);
    if (!writing) return 0;
    if (v->gate_team.window)
        return bfm_forest_message_art_draw_placed(writing, notice, v->gate_team.window_offset,
                                                 v->gate_team.window_right, pixels);
    const BfmBedroomRoot *speaker = NULL;
    if (bfm_valley_knitter_window(v->knitter, &speaker) == BFM_KNITTER_WINDOW_CAPTIVE) {
        const BfmBedroomView *view = bfm_hells_valley_projection(v->view);
        int actor[3], hero[3];
        if (!speaker || !bfm_bedroom_project(view, speaker->xyz, actor) ||
            !bfm_bedroom_project(view, bfm_hells_valley_player(v)->root.xyz, hero)) return 0;
        return bfm_forest_message_art_draw_placed(writing, notice, (int16_t)(actor[0] - 160),
                                                 actor[0] >= hero[0], pixels);
    }
    bfm_forest_message_art_draw(writing, notice, pixels);
    return 1;
}

int bfm_hells_valley_recover(const BfmHellsValley *v, const BfmGameplayRun *run, BfmBedroomEntry *out)
{
    return v && bfm_valley_defeat_destination(v->defeat, run, out);
}

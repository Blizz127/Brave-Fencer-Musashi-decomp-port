/* SPDX-License-Identifier: MIT */
#include "woods_box_visit.h"
#include "forest_message_art.h"
#include "field_rest.h"
#include "village_clock.h"
#include "woods_3026_box_art.h"
#include "woods_3026_box_hit.h"
#include "woods_pole_pickup_art.h"
#include "woods_box_return.h"
#include "woods_box_view.h"
#include "woods_rootrick_encounter.h"
#include "woods_rootrick_boulder.h"
#include "woods_earth_shrine.h"
#include "woods_earth_shrine_visit.h"
#include "woods_twinpeak_player_blade.h"
#include "bedroom_effects.h"
#include "village_field_defeat.h"
#include "woods_fusion_art.h"
#include "woods_player_hit.h"
#include "woods_revive.h"
#include <stdlib.h>
#include <string.h>

struct BfmWoodsBoxVisit {
    const MusashiDiscMedia *disc;
    BfmBedroomEntry *entry;
    const uint8_t *main;
    size_t main_bytes;
    uint16_t (*tiles)[1024];
    BfmVillageMotion *motion;
    BfmWoodsBoxView *view;
    BfmWoodsRootrickEncounter *rootrick;
    BfmEarthShrine *shrine;
    BfmEarthShrineVisit *ritual;
    BfmWoodsCasterMelee *blade;
    BfmWoodsCasterMeleeFrame swing;
    BfmFieldRest rest;
    BfmWoods3026BoxArt *box;
    BfmForestMessageArt *writing;
    BfmWoodsBoxRoute route;
    BfmWoods3026BoxHit hit;
    BfmWoodsBoxUse use;
    BfmWoodsPolePickups pickups;
    BfmWoodsPolePickupArt pickup_art;
    BfmWoodsPlayerHit *receiver;
    BfmBoulderImpact incoming;
    BfmWoodsPlayerHitFrame reaction;
    int block_revival, suppress_hurt, boulder_receiver;
    BfmForestDamageDigits water_digits;
    BfmVillageFieldDefeat defeat_actor;
    BfmRetryScreen *retry;
    size_t scene_bytes;
    int16_t water_contact[3];
    int in_water;
    enum { ALIVE, CHECK_STOCK, DEFEATED } life;
    unsigned age, actor_tick, sound;
    int box_cleanup;
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
};

void bfm_woods_box_visit_free(BfmWoodsBoxVisit *visit)
{
    if (!visit) return;
    bfm_retry_screen_free(visit->retry);
    bfm_woods_player_hit_free(visit->receiver);
    bfm_forest_message_art_free(visit->writing);
    bfm_woods_rootrick_free(visit->rootrick);
    bfm_woods_caster_melee_free(visit->blade);
    bfm_earth_shrine_free(visit->shrine);
    bfm_earth_visit_free(visit->ritual);
    bfm_woods_box_view_free(visit->view);
    bfm_woods_3026_box_art_free(visit->box);
    bfm_village_motion_free(visit->motion);
    free(visit);
}

const BfmVillagePlayer *bfm_woods_box_visit_player(const BfmWoodsBoxVisit *visit)
{
    return visit ? bfm_village_motion_player(visit->motion) : NULL;
}

int bfm_woods_box_visit_boulder_hit(BfmWoodsBoxVisit *visit, const BfmBoulderImpact *impact)
{
    const BfmVillagePlayer *p = bfm_woods_box_visit_player(visit);
    if (!p || !impact || visit->life != ALIVE || visit->block_revival ||
        !impact->block_revival || !(impact->hit_flags & 0xff)) return 0;
    int64_t z = (int64_t)p->root.xyz[2] + impact->hero_z_step;
    if (z < INT16_MIN || z > INT16_MAX) return 0;
    int16_t shifted[3] = {p->root.xyz[0], p->root.xyz[1], (int16_t)z};
    if (!bfm_village_motion_correct_contact(visit->motion, shifted)) return 0;
    visit->block_revival = 1;
    visit->incoming = *impact;
    return 1;
}

const BfmVillageMotion *bfm_woods_box_visit_departure(const BfmWoodsBoxVisit *visit)
{
    if (!visit || !bfm_village_motion_transfer(visit->motion)) return NULL;
    return visit->motion;
}

static int submit_box(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const BfmWoodsBoxVisit *visit = context;
    if (!bfm_earth_visit_draw(visit->ritual, visit->age, figure, view)) return 0;
    if (bfm_earth_visit_dark(visit->ritual)) return 1;
    return bfm_woods_3026_box_art_draw(visit->box, figure, view, visit->use.openness) &&
           bfm_woods_pole_pickup_art_add(&visit->pickup_art, &visit->pickups, figure, view,
                                         visit->actor_tick) &&
           bfm_woods_rootrick_draw(visit->rootrick, figure, view) &&
           bfm_woods_fusion_art_append(figure, view, NULL, NULL, visit->reaction.digits) &&
           bfm_woods_fusion_art_append(figure, view, NULL, NULL, &visit->water_digits);
}

int bfm_woods_box_visit_draw(BfmWoodsBoxVisit *visit, const uint8_t flags[64],
                             const BfmForestMessageFrame *window, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_woods_box_visit_player(visit);
    if (!p || !flags || !pixels) return 0;
    unsigned scale = visit->entry->state.progress[0x23] & 0x80 ?
                     BFM_HERO_MINI_SCALE : BFM_HERO_NORMAL_SCALE;
    BfmBedroomPose shown = p->pose;
    if (!bfm_twinpeak_player_blade_pose(&visit->swing, visit->main, visit->main_bytes, &shown)) return 0;
    bfm_earth_visit_pose(visit->ritual, &shown);
    if (visit->reaction.pose &&
        !bfm_woods_fusion_art_pose(visit->main, visit->main_bytes, visit->reaction.pose,
                                   bfm_village_field_visible(&shown, flags), shown.hide_fusion, &shown))
        return 0;
    if (!bfm_village_motion_face(visit->motion, visit->tiles) ||
        !bfm_woods_box_view_draw_presentation(visit->view, &p->root, &shown, flags, visit->age, scale,
                                bfm_earth_visit_dark(visit->ritual), pixels,
                                submit_box, visit)) return 0;
    if (window && bfm_woods_rootrick_controls_player(visit->rootrick))
        return bfm_woods_rootrick_window(visit->rootrick, visit->writing, window, pixels);
    if (window && (bfm_earth_shrine_in_scene(visit->shrine) || bfm_earth_visit_controls(visit->ritual)))
        return bfm_forest_message_art_draw_placed(visit->writing, window, 0, 0, pixels);
    if (window) bfm_forest_message_art_draw(visit->writing, window, pixels);
    return 1;
}

BfmWoodsBoxVisit *bfm_woods_box_visit_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    if (!disc || !entry || entry->scene_member != 11 || !main || !main_bytes || !tiles) return NULL;
    BfmWoodsBoxVisit *visit = calloc(1, sizeof(*visit));
    if (!visit) return NULL;
    size_t bytes = bfm_read_archive_field_scene(disc, "SC02.CD", 11, visit->scene);
    visit->disc = disc;
    visit->scene_bytes = bytes;
    const size_t hurt_palette = UINT32_C(0x80193e95) - UINT32_C(0x80128158);
    if (hurt_palette >= bytes) goto fail;
    visit->receiver = bfm_woods_player_hit_open_palette(disc, visit->scene[hurt_palette]);
    if (!visit->receiver) goto fail;
    const uint8_t *destination = entry->state.destination;
    if (!bfm_woods_box_route_open(&visit->route, 0x3026, visit->scene, bytes) ||
        !bfm_woods_box_arrival(&visit->route, destination) ||
        !bfm_woods_pole_pickups_load(&visit->pickups, visit->scene, bytes) ||
        !bfm_woods_pole_pickup_art_load(&visit->pickup_art, visit->scene, bytes) ||
        !bfm_woods_3026_box_hit_mount(visit->scene, bytes, &visit->hit)) goto fail;
    visit->motion = bfm_village_motion_box_field(visit->scene, bytes, main, main_bytes, destination);
    const BfmVillagePlayer *p = bfm_woods_box_visit_player(visit);
    if (!p || !p->pose.pose || !bfm_village_motion_box_controls(visit->motion, disc)) goto fail;
    visit->box = bfm_woods_3026_box_art_load(visit->scene, bytes);
    if (!visit->box) goto fail;
    visit->view = bfm_woods_box_view_open(disc, visit->scene, bytes, entry->state.progress[0x37], tiles);
    if (!visit->view) goto fail;
    visit->rootrick = bfm_woods_rootrick_open(disc, visit->scene, bytes, main, main_bytes);
    if (!visit->rootrick) goto fail;
    visit->blade = bfm_woods_twinpeak_melee_open(disc, visit->scene, bytes, main, main_bytes);
    if (!visit->blade) goto fail;
    int earned_scroll = entry->state.flags[22 / 8] & (1u << (22 % 8));
    visit->shrine = bfm_earth_shrine_open(visit->scene, bytes, earned_scroll);
    if (!visit->shrine) goto fail;
    if (!earned_scroll) {
        visit->ritual = bfm_earth_visit_open(disc, visit->scene, bytes, main, main_bytes, tiles,
                                            visit->motion, visit->view, &entry->state);
        if (!visit->ritual) goto fail;
    }
    /* The room supplies its own dialogue border insets. */
    const size_t inset = UINT32_C(0x80191400) - UINT32_C(0x80128158);
    visit->writing = bfm_castle_message_art_load(disc, visit->scene, bytes, tiles, inset);
    if (!visit->writing) goto fail;
    visit->entry = entry;
    visit->main = main;
    visit->main_bytes = main_bytes;
    visit->tiles = tiles;
    bfm_woods_box_view_follow(visit->view, p->root.xyz, 1, 0);
    /* Qualify the actual receiving scene before the caller retires its old room. */
    uint32_t (*trial)[320] = malloc(240 * sizeof(*trial));
    int drawable = trial && bfm_woods_box_visit_draw(visit, entry->state.flags, NULL, trial);
    free(trial);
    if (drawable) return visit;
fail:
    bfm_woods_box_visit_free(visit);
    return NULL;
}

int bfm_woods_box_visit_close(BfmWoodsBoxVisit *visit, unsigned answer)
{
    if (!visit) return 0;
    if (bfm_woods_rootrick_closed(visit->rootrick)) return 1;
    if (bfm_earth_shrine_closed(visit->shrine) || bfm_earth_visit_close(visit->ritual)) return 1;
    if (!visit->use.wait_for_answer) return 0;
    bfm_woods_box_use_close(&visit->use, answer);
    return 1;
}

int bfm_woods_box_visit_take_hud(BfmWoodsBoxVisit *visit)
{
    if (!visit) return 0;
    int hud = bfm_earth_visit_hud(visit->ritual);
    return hud ? hud : bfm_woods_rootrick_take_hud(visit->rootrick);
}

int bfm_woods_box_visit_rest_hud(const BfmWoodsBoxVisit *visit, BfmForestHud *hud)
{
    if (!visit || !hud) return 0;
    const uint8_t *status = visit->entry->state.progress;
    unsigned palette;
    if (bfm_village_hud_step_blade(hud, status[0x48], &palette) &&
        !bfm_village_motion_twinpeak_orb(visit->motion, palette, visit->tiles)) return 0;
    bfm_village_hud_rest(hud, (status[0x48] & 0x80) != 0, status[0x47],
                         bfm_field_rest_sleeping(&visit->rest));
    return 1;
}

void bfm_woods_box_visit_score_applied(BfmWoodsBoxVisit *visit, unsigned selector)
{
    if (visit) {
        bfm_woods_rootrick_score_applied(visit->rootrick, selector);
        bfm_earth_shrine_score_applied(visit->shrine, selector);
    }
}

unsigned bfm_woods_box_visit_score(const BfmWoodsBoxVisit *visit)
{
    if (!visit) return 0;
    unsigned field = bfm_earth_shrine_score(visit->shrine, bfm_woods_rootrick_score(visit->rootrick));
    return bfm_earth_visit_score(visit->ritual, field);
}

int bfm_woods_box_visit_take_retired_window(BfmWoodsBoxVisit *visit)
{
    return visit && bfm_woods_rootrick_take_retired_window(visit->rootrick);
}

unsigned bfm_woods_box_visit_take_voice(BfmWoodsBoxVisit *visit)
{
    return visit ? bfm_woods_rootrick_take_voice(visit->rootrick) : 0;
}

unsigned bfm_woods_box_visit_take_sound(BfmWoodsBoxVisit *visit)
{
    if (!visit) return 0;
    unsigned sound = visit->sound;
    visit->sound = 0;
    if (!sound) sound = bfm_earth_visit_sound(visit->ritual);
    return sound ? sound : bfm_woods_rootrick_take_sound(visit->rootrick);
}

int bfm_woods_box_visit_begin_retry(BfmWoodsBoxVisit *visit, BfmGameplayRun *run)
{
    if (!visit || visit->life != DEFEATED || visit->retry) return 1;
    visit->defeat_actor = (BfmVillageFieldDefeat){
        .field=bfm_woods_box_view_field(visit->view), .motion=visit->motion,
        .save=&visit->entry->state, .main=visit->main, .main_bytes=visit->main_bytes,
        .tiles=visit->tiles, .age=&visit->age
    };
    const size_t table = UINT32_C(0x801930a0) - UINT32_C(0x80128158);
    visit->retry = bfm_village_field_defeat_open(&visit->defeat_actor, visit->disc, visit->scene,
        visit->scene_bytes, table, bfm_woods_box_view_camera(visit->view), run);
    return visit->retry != NULL;
}

BfmRetryScreen *bfm_woods_box_visit_retry(const BfmWoodsBoxVisit *visit)
{
    return visit ? visit->retry : NULL;
}

int bfm_woods_box_visit_draw_retry(BfmWoodsBoxVisit *visit, const BfmForestHud *hud,
                                  uint32_t pixels[240][320])
{
    if (!visit || !visit->retry) return 0;
    if (bfm_retry_screen_keep_world(visit->retry) &&
        !bfm_woods_box_visit_draw(visit, visit->entry->state.flags, NULL, pixels)) return 0;
    return bfm_retry_screen_draw(visit->retry, hud, visit->writing, pixels);
}

int bfm_woods_box_visit_prepare_recovery(const BfmWoodsBoxVisit *visit, const BfmGameplayRun *run,
                                         BfmBedroomEntry *out)
{
    if (!visit || !visit->retry) return 0;
    const size_t destination = UINT32_C(0x8019328c) - UINT32_C(0x80128158);
    BfmWoodsRetryPage page = bfm_retry_screen_woods_destination(visit->retry);
    return bfm_woods_retry_field_destination(visit->disc, run, page, visit->scene,
                                            visit->scene_bytes, destination, out);
}

static int water_contact(BfmWoodsBoxVisit *visit, BfmGameplayRun *run, const BfmVillagePlayer *p,
                         int blinking)
{
    const int16_t *splash = NULL;
    if (p->events & BFM_VILLAGE_WATER_HIT) {
        memcpy(visit->water_contact, p->root.xyz, sizeof visit->water_contact);
        visit->in_water = 1;
    }
    if (p->events & BFM_VILLAGE_WATER_SPLASH) {
        if (run) {
            const uint8_t *player = run->player.record;
            int guarded = player[0x49] == 0x10 &&
                          (player[0x50] || player[0x51] || player[0x52] || player[0x53]);
            if (!bfm_woods_player_hit_water_entry(visit->receiver, run, visit->water_contact,
                    guarded ? UINT32_C(0x800) : 0, &visit->water_digits)) return 0;
            splash = visit->water_contact;
        }
        visit->sound = BFM_WOODS_WATER_ENTRY_SOUND;
    }
    if (p->events & BFM_VILLAGE_WATER_RETURN) {
        if (run) {
            BfmWoodsWaterLanding landing = bfm_woods_player_hit_water_return(visit->receiver, run);
            if (landing == BFM_WOODS_WATER_NO_LANDING) return 0;
            memcpy(visit->entry->state.progress, run->player.record, sizeof run->player.record);
            if (landing == BFM_WOODS_WATER_ZERO_HP) visit->life = CHECK_STOCK;
        }
        visit->in_water = 0;
    }
    return bfm_woods_box_view_water_step(visit->view, splash, p->water_hidden || blinking,
                                         p->water_hidden, run);
}

static BfmWoodsPlayerHitInput player_hit_input(BfmWoodsBoxVisit *visit)
{
    BfmWoodsPlayerHitInput hit = {.hit_flags=visit->incoming.hit_flags,
        .power=visit->incoming.power, .attacker_yaw=visit->incoming.attacker_yaw};
    const BfmVillagePlayer *p = bfm_woods_box_visit_player(visit);
    hit.root = &p->root;
    hit.protected_action = bfm_woods_rootrick_player_protection(visit->rootrick);
    if (!hit.protected_action && !p->controlled) hit.protected_action = 1;
    hit.receives_damage = p->controlled;
    hit.airborne = bfm_village_motion_box_air_action(visit->motion);
    hit.player_flags = bfm_village_motion_box_pole(visit->motion) ? 0x10u : 0;
    hit.player_action = visit->reaction.pose ? 25u : 0;
    return hit;
}

static int player_hit_step(BfmWoodsBoxVisit *visit, BfmGameplayRun *run)
{
    bfm_forest_damage_digits_advance(&visit->water_digits);
    if (!run) return 1;
    BfmWoodsPlayerHitInput hit = player_hit_input(visit);
    BfmWoodsPlayerHitFrame *f = &visit->reaction;
    if (!bfm_woods_player_hit_step(visit->receiver, run, &hit, f)) return 0;
    visit->incoming = (BfmBoulderImpact){0};
    if (f->hurt_started) visit->boulder_receiver = 1;
    if (f->sound == BFM_WOODS_HURT_SOUND && visit->suppress_hurt)
        visit->suppress_hurt = 0;
    else if (f->sound)
        visit->sound = f->sound;
    if (f->movement_locked) {
        BfmWoodsMotionContact ground;
        if (!bfm_village_motion_woods_reaction(visit->motion, NULL,
                f->hurt_started ? BFM_WOODS_HURT_START : BFM_WOODS_HURT_HOLD, 0, 0, &ground))
            return 0;
        BfmWoodsPlayerHitContact contact = BFM_WOODS_HIT_SAME_SUPPORT;
        if (ground.lost_support) contact = BFM_WOODS_HIT_LOST_SUPPORT;
        else if (ground.slope & 0x4000) contact = BFM_WOODS_HIT_SURFACE_REACTION;
        else if (ground.landed) contact = BFM_WOODS_HIT_LANDED;
        bfm_woods_player_hit_hurt_contact(visit->receiver, run, hit.player_flags, contact, f);
        if (!bfm_village_motion_combat_face(visit->motion, f->pose ? 22 : 17)) return 0;
        if (f->zero_hp || f->recovered) {
            if (!bfm_village_motion_woods_finish_hurt(visit->motion, bfm_field_tired_stance(run->player.record)))
                return 0;
            if (f->zero_hp) visit->life = CHECK_STOCK;
        }
    }
    memcpy(visit->entry->state.progress, run->player.record, sizeof run->player.record);
    return 1;
}

static int encounter_step(BfmWoodsBoxVisit *visit, BfmGameplayRun *run, BfmFieldInput input,
                          unsigned actor_tick, BfmForestMessage **opened)
{
    int stand = visit->boulder_receiver ? bfm_village_motion_box_reported_stand(visit->motion) : -1;
    if (!bfm_woods_rootrick_step(visit->rootrick, visit->motion, visit->view, run,
                                 &visit->entry->state, input, actor_tick, stand, opened)) return 0;
    if (bfm_woods_rootrick_take_hurt_suppression(visit->rootrick)) visit->suppress_hurt = 1;
    BfmBoulderImpact impact;
    return !bfm_woods_rootrick_take_boulder_hit(visit->rootrick, &impact) ||
           bfm_woods_box_visit_boulder_hit(visit, &impact);
}

static int step_room(BfmWoodsBoxVisit *visit, BfmGameplayRun *run, BfmFieldInput input,
                     unsigned actor_tick, BfmForestMessage **opened, unsigned *pickup_amount)
{
    const BfmVillagePlayer *p = bfm_woods_box_visit_player(visit);
    if (!p || !opened) return 0;
    *opened = NULL;
    BfmSaveRecord *live = &visit->entry->state;
    BfmWoodsPlayerHitInput hit = player_hit_input(visit);
    if (visit->life == CHECK_STOCK && bfm_woods_player_hit_can_check_defeat(run, &hit)) {
        unsigned slot;
        BfmWoodsReviveResult result = bfm_woods_revive_prepare(run, live, visit->block_revival, visit->main,
                                                               visit->main_bytes, &slot, opened);
        if (result == BFM_WOODS_REVIVE_ERROR) return 0;
        if (result == BFM_WOODS_REVIVE_UNAVAILABLE) {
            visit->life = DEFEATED;
            bfm_woods_player_hit_blink(visit->receiver, 0);
            /* The Retry renderer now draws the collapse over the room's field. */
            return bfm_woods_box_view_water_step(visit->view, NULL, 1, 0, run);
        }
        if (!bfm_village_motion_woods_revive(visit->motion, bfm_field_tired_stance(run->player.record))) {
            bfm_forest_message_free(*opened);
            *opened = NULL;
            return 0;
        }
        bfm_woods_revive_restore(run, live, slot);
        input = (BfmFieldInput){0};
        visit->life = ALIVE;
    }
    if (visit->life == DEFEATED) return 1;
    /* The player's clock precedes Square and the Box actor. The actor needs
     * one final update after the terminal lid wait to release the player. */
    ++visit->age;
    if (bfm_earth_visit_controls(visit->ritual)) {
        /* The accepted strike keeps its recovery frames while the child begins shaking. */
        int recovering = visit->swing.action != 0;
        if (recovering) {
            if (!bfm_twinpeak_player_blade_step(visit->blade, visit->motion, live, run,
                                                (BfmFieldInput){0}, 1, &visit->swing) ||
                !bfm_village_motion_box_blade_hold(visit->motion, !visit->swing.action)) return 0;
            if (visit->swing.sound) visit->sound = visit->swing.sound;
        }
        return bfm_earth_visit_step(visit->ritual, run, actor_tick, !recovering, opened);
    }
    if (bfm_earth_shrine_in_scene(visit->shrine))
        return bfm_earth_shrine_update(visit->shrine, visit->motion, visit->view, opened);
    if (bfm_woods_rootrick_controls_player(visit->rootrick)) {
        if (!player_hit_step(visit, run)) return 0;
        if (visit->boulder_receiver && !visit->reaction.movement_locked) {
            /* A pending collapse can outlive the reaction. The scene's button lock
             * leaves the ordinary fall, landing and supported stance running. */
            const BfmVillageCamera *camera = bfm_woods_box_view_camera(visit->view);
            if (!bfm_village_motion_box_step(visit->motion, 0, 0, (camera->yaw + 2048) & 4095,
                                             0, 0, run) ||
                !water_contact(visit, run, p, visit->reaction.hide_body)) return 0;
        } else if (!bfm_woods_box_view_water_step(visit->view, NULL, visit->reaction.hide_body, 0, run)) {
            return 0;
        }
        if (visit->reaction.movement_locked) input = (BfmFieldInput){0};
        return encounter_step(visit, run, input, actor_tick, opened);
    }
    if (visit->box_cleanup) {
        visit->box_cleanup = 0;
        return 1;
    }
    int use_box = 0;
    if (!visit->use.route && visit->rest.pose == BFM_FIELD_REST_FREE && !visit->swing.action &&
        run && p->controlled && input.square_pressed && bfm_village_motion_grounded(visit->motion) &&
        !bfm_woods_box_visit_departure(visit)) {
        BfmForestTransform model;
        if (!bfm_bedroom_figure_actor_root(&p->root, p->pose.pitch, &model)) return 0;
        unsigned flags = live->progress[0x23] & 0x80 ? 0x400u : 0;
        use_box = bfm_woods_3026_box_can_use(&visit->hit, &input, &model, p->root.xyz, flags, p->root.busy);
    }
    int resting = 0;
    if (!visit->use.route && !bfm_woods_box_visit_departure(visit)) {
        /* An ordinary standing player selects the rest stance before R1 starts
         * charging. The same Select edge must not also toggle the blade. */
        if (!use_box && visit->life == ALIVE && !visit->swing.action && !visit->in_water &&
            !visit->reaction.movement_locked &&
            !visit->incoming.hit_flags && !(live->progress[0x23] & 0x80) &&
            bfm_twinpeak_player_field_ready(visit->motion, run)) {
            bfm_village_motion_woods_condition(visit->motion, bfm_field_tired_stance(live->progress));
            bfm_field_rest_input(&visit->rest, live, input);
            input.select_pressed = 0;
            resting = visit->rest.pose != BFM_FIELD_REST_FREE;
        } else if (visit->rest.pose != BFM_FIELD_REST_FREE) {
            visit->rest = (BfmFieldRest){0};
            live->progress[0x47] = 0;
        }
        if ((p->controlled || visit->in_water || bfm_village_motion_ingress_clock_running(visit->motion)) &&
            !bfm_village_motion_woods_departing(visit->motion)) {
            int asleep = bfm_field_rest_sleeping(&visit->rest);
            if (run && p->controlled) bfm_field_rest_stamina(live, asleep);
            bfm_village_clock_step(live, asleep);
        }
        if (run) memcpy(run->player.record, live->progress, sizeof run->player.record);
    }
    if (resting) input = (BfmFieldInput){0};
    if (use_box && !bfm_woods_box_use_begin(&visit->use, &visit->route, visit->main, visit->main_bytes))
        return 0;
    if (visit->use.route) {
        unsigned cue;
        if (!bfm_woods_box_use_step(&visit->use, run, live, opened, &cue)) return 0;
        if (cue) visit->sound = cue;
        if (!visit->use.route) visit->box_cleanup = 1;
        return 1;
    }
    const BfmVillageCamera *camera = bfm_woods_box_view_camera(visit->view);
    int movement_yaw = (camera->yaw + 2048) & 4095;
    if (bfm_woods_box_visit_departure(visit)) {
        /* The source field keeps its virtual walk while the loader takes
         * control. Physical stick and shoulder changes no longer apply. */
        if (!bfm_village_motion_woods_departure_step(visit->motion, movement_yaw, run)) return 0;
        p = bfm_woods_box_visit_player(visit);
        bfm_woods_box_view_follow(visit->view, p->root.xyz, 0, 0);
        return 1;
    }
    if (!player_hit_step(visit, run)) return 0;
    bfm_woods_box_view_light(visit->view, live->progress[0x37]);
    int16_t previous[3];
    memcpy(previous, p->root.xyz, sizeof previous);
    bfm_village_motion_water_bypass(visit->motion, live->variables[0xcf] != 0);
    int previous_action = visit->swing.action;
    int field_free = !resting && !visit->reaction.movement_locked && !visit->in_water && visit->life == ALIVE;
    if (run && !bfm_twinpeak_player_blade_step(visit->blade, visit->motion, live, run, input,
                                               field_free, &visit->swing)) return 0;
    int blade_holds = field_free && (visit->swing.action || previous_action == 6) &&
                      bfm_village_motion_grounded(visit->motion) &&
                      !bfm_village_motion_woods_sliding(visit->motion);
    if (resting) {
        int result = bfm_village_motion_field_rest(visit->motion, visit->rest.pose, visit->rest.ticks);
        if (!result) return 0;
        if (result == 2 && visit->rest.pose == BFM_FIELD_REST_WAKE) {
            live->progress[0x47] = 0;
            run->player.record[0x47] = 0;
        }
        bfm_field_rest_advance(&visit->rest, result == 2);
    } else if (blade_holds) {
        if (!bfm_village_motion_box_blade_hold(visit->motion, !visit->swing.action)) return 0;
        if (!bfm_village_motion_grounded(visit->motion) || bfm_village_motion_woods_sliding(visit->motion)) {
            if (!bfm_twinpeak_player_blade_step(visit->blade, visit->motion, live, run,
                                                (BfmFieldInput){0}, 0, &visit->swing)) return 0;
        }
        if (visit->swing.sound) visit->sound = visit->swing.sound;
    } else if (!visit->reaction.movement_locked &&
        !bfm_village_motion_box_step(visit->motion, input.x, input.y, movement_yaw,
                                    input.cross_pressed, input.cross_held, run)) return 0;
    p = bfm_woods_box_visit_player(visit);
    if (!water_contact(visit, run, p, visit->reaction.hide_body)) return 0;
    int16_t foot[3];
    memcpy(foot, p->root.xyz, sizeof foot);
    if (p->controlled && bfm_woods_3026_box_body(&visit->hit, actor_tick, previous, foot) &&
        !bfm_village_motion_correct_contact(visit->motion, foot)) return 0;
    if (run && p->controlled && visit->life == ALIVE && !bfm_woods_box_visit_departure(visit)) {
        unsigned cue = bfm_woods_pole_pickups_step(&visit->pickups, &run->player, p->root.xyz,
                                                  pickup_amount);
        if (cue) {
            memcpy(live->progress + 0x40, run->player.record + 0x40, sizeof(uint16_t));
            visit->sound = cue;
        }
    }
    bfm_woods_box_view_follow_grip(visit->view, p->root.xyz, 0, input.camera_pressed,
                                   bfm_village_motion_box_pole(visit->motion) != 0);
    if (run && !visit->in_water && visit->life == ALIVE && !bfm_woods_box_visit_departure(visit) &&
        !bfm_earth_shrine_on_approach(visit->shrine, &p->root)) return 0;
    if (run && !bfm_earth_shrine_in_scene(visit->shrine) &&
        !bfm_earth_visit_strike(visit->ritual, run, actor_tick, &visit->swing, opened)) return 0;
    return !run || bfm_earth_visit_controls(visit->ritual) || encounter_step(visit, run, input, actor_tick, opened);
}

int bfm_woods_box_visit_step_at(BfmWoodsBoxVisit *visit, BfmGameplayRun *run,
                                BfmFieldInput input, unsigned actor_tick, BfmForestMessage **opened)
{
    unsigned pickup_amount = 0;
    if (!visit || (run && !bfm_woods_rootrick_update_ledges(visit->rootrick, run))) return 0;
    if (!step_room(visit, run, input, actor_tick, opened, &pickup_amount)) return 0;
    if (run && visit->swing.action &&
        (bfm_earth_shrine_in_scene(visit->shrine) ||
         bfm_woods_rootrick_controls_player(visit->rootrick) ||
         visit->in_water || visit->life != ALIVE || bfm_woods_box_visit_departure(visit))) {
        if (!bfm_twinpeak_player_blade_step(visit->blade, visit->motion, &visit->entry->state,
                                            run, (BfmFieldInput){0}, 0, &visit->swing)) return 0;
    }
    bfm_earth_visit_camera(visit->ritual);
    visit->actor_tick = actor_tick;
    const BfmVillagePlayer *p = bfm_woods_box_visit_player(visit);
    bfm_woods_pole_pickup_art_step(&visit->pickup_art, p->root.xyz, pickup_amount);
    return 1;
}

int bfm_woods_box_visit_step(BfmWoodsBoxVisit *visit, BfmGameplayRun *run,
                             BfmFieldInput input, BfmForestMessage **opened)
{
    return visit && bfm_woods_box_visit_step_at(visit, run, input, visit->age, opened);
}

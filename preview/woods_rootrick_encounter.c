/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_encounter.h"
#include "woods_rootrick_actor.h"
#include "woods_rootrick_art.h"
#include "woods_rootrick_blocking.h"
#include "woods_rootrick_challenge.h"
#include "woods_rootrick_stage.h"
#include "woods_rootrick_notice.h"
#include "woods_rootrick_wall.h"
#include "woods_rootrick_ledge_art.h"
#include "woods_rootrick_impact.h"
#include "woods_rootrick_wall_effects.h"
#include "woods_rootrick_summit.h"
#include "forest_pose.h"
#include "bedroom_effects.h"
#include <stdlib.h>

enum EncounterPhase { FIELD, STAGING, CHALLENGE, TO_WALL, RACING, CREST, OUTCOME, FINISHED };
/* Both supported entrances (the west approach and the Memory Box) select Twinpeak. */
enum { TWINPEAK_SCORE = 0x18 };

struct BfmWoodsRootrickEncounter {
    const MusashiDiscMedia *disc;
    const uint8_t *scene;
    size_t bytes, main_bytes;
    const uint8_t *main;
    BfmWoodsRootrickTrigger trigger;
    BfmWoodsRootrickStage staging;
    BfmWoodsRootrickActor actor;
    BfmWoodsRootrickImpact impact;
    BfmWoodsRootrickWallArt chips;
    BfmWoodsRootrickSummit *summit;
    BfmBoulderImpact player_hit;
    int pending_hit, suppress_hurt, return_camera, restart_follow, retire_retry;
    unsigned voice;
    BfmRootrickWall *wall;
    BfmWoodsRootrickLedgeArt *ledges;
    BfmRootrickRaceOutcome outcome;
    int rival_attached, retry_window, shadow;
    BfmRootrickChallenge script;
    BfmWoodsRootrickArt *art;
    BfmWoodsRootrickBlocking *blocking;
    enum EncounterPhase phase;
    unsigned score, sounds[4], sounds_issued, sounds_taken;
    enum { RESTORE_IDLE, RESTORE_REQUESTED, RESTORE_INSTALLED } room_score;
    BfmForestMarker notice;
    struct { BfmRootrickEvent kind; uint32_t operand; } player_orders[4];
    unsigned player_order_begin, player_order_count;
    int hud_request, visible, window, window_offset, window_right, race_camera, fixed_shot;
};

typedef struct {
    BfmWoodsRootrickEncounter *encounter;
    BfmWoodsBoxView *view;
    BfmVillageMotion *motion;
    BfmForestMessage **opened;
} SceneUpdate;

BfmWoodsRootrickEncounter *bfm_woods_rootrick_open(const MusashiDiscMedia *disc,
                                                  const uint8_t *scene, size_t bytes,
                                                  const uint8_t *main, size_t main_bytes)
{
    if (!disc || !scene) return NULL;
    BfmWoodsRootrickEncounter *e = calloc(1, sizeof *e);
    if (!e || !bfm_woods_rootrick_trigger_load(&e->trigger, scene, bytes) ||
        !bfm_woods_rootrick_impact_load(&e->impact, scene, bytes, main, main_bytes) ||
        !bfm_woods_rootrick_wall_art_load(&e->chips, scene, bytes)) {
        free(e);
        return NULL;
    }
    e->wall = bfm_rootrick_wall_open(scene, bytes);
    e->ledges = bfm_woods_rootrick_ledge_art_open(scene, bytes, e->wall);
    if (!e->ledges) {
        bfm_rootrick_wall_free(e->wall);
        free(e);
        return NULL;
    }
    e->disc = disc;
    e->main = main;
    e->main_bytes = main_bytes;
    e->scene = scene;
    e->bytes = bytes;
    e->score = TWINPEAK_SCORE;
    return e;
}

void bfm_woods_rootrick_free(BfmWoodsRootrickEncounter *e)
{
    if (!e) return;
    bfm_woods_rootrick_summit_free(e->summit);
    bfm_woods_rootrick_ledge_art_free(e->ledges);
    bfm_rootrick_wall_free(e->wall);
    bfm_woods_rootrick_art_free(e->art);
    bfm_woods_rootrick_blocking_free(e->blocking);
    free(e);
}

int bfm_woods_rootrick_controls_player(const BfmWoodsRootrickEncounter *e)
{
    return e && e->phase != FIELD && e->phase != FINISHED;
}

int bfm_woods_rootrick_draw(const BfmWoodsRootrickEncounter *e,
                           BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!e) return 1;
    if (!bfm_woods_rootrick_ledge_art_add(e->ledges, figure, view)) return 0;
    if ((e->visible || e->shadow) &&
        !bfm_woods_rootrick_art_visible(e->art, figure, view, e->actor.xyz, e->actor.yaw,
                                       e->actor.pitch, e->actor.clip, e->actor.elapsed,
                                       e->visible, e->shadow)) return 0;
    return bfm_woods_rootrick_summit_draw(e->summit, figure, view) &&
           bfm_woods_rootrick_wall_art_add(&e->chips, figure, view) &&
           bfm_woods_rootrick_impact_submit(&e->impact, figure, view) &&
           (!e->notice.reaction.scale || bfm_woods_rootrick_notice_add(&e->notice, figure, view));
}

int bfm_woods_rootrick_closed(BfmWoodsRootrickEncounter *e)
{
    if (!e || !e->window) return 0;
    if (e->retry_window) {
        bfm_rootrick_wall_retry_closed(e->wall);
        e->retry_window = 0;
    }
    e->window = 0;
    return 1;
}

int bfm_woods_rootrick_window(const BfmWoodsRootrickEncounter *e,
                             const BfmForestMessageArt *art, const BfmForestMessageFrame *window,
                             uint32_t pixels[240][320])
{
    if (!e || !e->window || !art || !window || !pixels) return 0;
    if (e->retry_window) {
        bfm_bedroom_message_art_draw_side(art, window, 160, 0, pixels);
        return 1;
    }
    return bfm_forest_message_art_draw_placed(art, window, (int16_t)e->window_offset,
                                              e->window_right, pixels);
}

int bfm_woods_rootrick_take_hud(BfmWoodsRootrickEncounter *e)
{
    if (!e) return 0;
    int request = e->hud_request;
    e->hud_request = 0;
    return request;
}

static int add_sound(BfmWoodsRootrickEncounter *e, unsigned cue)
{
    if (!cue) return 1;
    if (e->sounds_issued == sizeof e->sounds / sizeof e->sounds[0]) return 0;
    e->sounds[e->sounds_issued++] = cue;
    return 1;
}

unsigned bfm_woods_rootrick_take_sound(BfmWoodsRootrickEncounter *e)
{
    return e && e->sounds_taken < e->sounds_issued ? e->sounds[e->sounds_taken++] : 0;
}

int bfm_woods_rootrick_take_retired_window(BfmWoodsRootrickEncounter *e)
{
    if (!e || !e->retire_retry) return 0;
    e->retire_retry = 0;
    return 1;
}

unsigned bfm_woods_rootrick_take_voice(BfmWoodsRootrickEncounter *e)
{
    if (!e) return 0;
    unsigned voice = e->voice;
    e->voice = 0;
    return voice;
}

int bfm_woods_rootrick_take_hurt_suppression(BfmWoodsRootrickEncounter *e)
{
    if (!e || !e->suppress_hurt) return 0;
    e->suppress_hurt = 0;
    return 1;
}

int bfm_woods_rootrick_take_boulder_hit(BfmWoodsRootrickEncounter *e, BfmBoulderImpact *hit)
{
    if (!e || !hit || !e->pending_hit) return 0;
    *hit = e->player_hit;
    e->pending_hit = 0;
    return 1;
}

uint32_t bfm_woods_rootrick_player_protection(const BfmWoodsRootrickEncounter *e)
{
    if (!bfm_woods_rootrick_controls_player(e)) return 0;
    /* In the field a nonzero script owner arbitrates input. The wall releases
     * normal action control while it still owns the attached player. */
    const uint32_t script_owner = UINT32_C(1);
    const uint32_t locks_action = UINT32_C(1) << 24;
    if (e->phase == RACING)
        return bfm_rootrick_wall_attached(e->wall) ? script_owner : script_owner | locks_action;
    return script_owner | locks_action | (UINT32_C(1) << 26) |
           bfm_woods_rootrick_summit_grants(e->summit);
}

unsigned bfm_woods_rootrick_score(const BfmWoodsRootrickEncounter *e)
{
    return e ? e->score : 0;
}

/* These script operations enqueue player actions. A released actor wait can
 * put the notice and a new look into the queue on the same field. Musashi's
 * next visit consumes at most one action after his previous order finishes. */
static int queue_player(BfmWoodsRootrickEncounter *e, const BfmRootrickCommand *cmd)
{
    unsigned capacity = sizeof e->player_orders / sizeof e->player_orders[0];
    if (e->player_order_count == capacity) return 0;
    unsigned slot = (e->player_order_begin + e->player_order_count) % capacity;
    e->player_orders[slot].kind = cmd->event;
    e->player_orders[slot].operand = cmd->operand;
    ++e->player_order_count;
    return 1;
}

static int service_player(BfmWoodsRootrickEncounter *e, const BfmVillageCamera *camera)
{
    const BfmBedroomRoot *root = bfm_woods_rootrick_player(e->blocking);
    if (!root) return 0;
    if (!root->busy && e->player_order_count) {
        unsigned slot = e->player_order_begin;
        uint32_t value = e->player_orders[slot].operand;
        int16_t x = (int16_t)value, z = (int16_t)(value >> 16);
        switch (e->player_orders[slot].kind) {
        case BFM_ROOTRICK_WALK_PLAYER:
            if (!bfm_woods_rootrick_walk(e->blocking, x, z)) return 0;
            break;
        case BFM_ROOTRICK_FACE_POINT:
            if (!bfm_woods_rootrick_face(e->blocking, x, z)) return 0;
            break;
        case BFM_ROOTRICK_FACE_ACTOR:
            if (!bfm_woods_rootrick_face(e->blocking, e->actor.xyz[0], e->actor.xyz[2])) return 0;
            break;
        case BFM_ROOTRICK_PLAYER_NOTICE:
            if (!bfm_woods_rootrick_notice_start(&e->notice, e->scene, e->bytes, root->xyz))
                return 0;
            break;
        default: return 0;
        }
        e->player_order_begin = (slot+1) % (sizeof e->player_orders / sizeof e->player_orders[0]);
        --e->player_order_count;
    }
    if (!bfm_woods_rootrick_step_player(e->blocking, camera)) return 0;
    bfm_forest_marker_advance(&e->notice, root->xyz);
    return 1;
}

static int script_command(void *context, const BfmRootrickCommand *cmd)
{
    SceneUpdate *update = context;
    BfmWoodsRootrickEncounter *e = update->encounter;
    const BfmVillageCamera *camera = bfm_woods_box_view_camera(update->view);
    const BfmVillagePlayer *hero = bfm_village_motion_player(update->motion);
    uint32_t value = cmd->operand;
    if (e->phase == OUTCOME) {
        int consumed = bfm_woods_rootrick_summit_command(e->summit, cmd);
        if (consumed) return consumed > 0;
    }
    switch (cmd->event) {
    case BFM_ROOTRICK_STAGE_SCORE:
        bfm_woods_rootrick_score_arm(&e->staging.score, value);
        e->score = 0;
        break;
    case BFM_ROOTRICK_ROOM_CAMERA:
        bfm_woods_rootrick_room_camera_begin(e->blocking, camera);
        e->fixed_shot = e->race_camera = 0;
        e->restart_follow = e->return_camera = 1;
        break;
    case BFM_ROOTRICK_HIDE_HUD: e->hud_request = -1; break;
    case BFM_ROOTRICK_SHOW_HUD: e->hud_request = 1; break;
    case BFM_ROOTRICK_WALK_PLAYER:
    case BFM_ROOTRICK_FACE_POINT:
    case BFM_ROOTRICK_FACE_ACTOR:
    case BFM_ROOTRICK_PLAYER_NOTICE:
        return queue_player(e, cmd);
    case BFM_ROOTRICK_CAMERA_SHOT:
        e->fixed_shot = 1;
        e->race_camera = 0;
        return bfm_woods_rootrick_frame(e->blocking,value,camera);
    case BFM_ROOTRICK_ACTOR_STAGE:
        return bfm_woods_rootrick_actor_stage(&e->actor,value);
    case BFM_ROOTRICK_BIND_POSE:
        bfm_woods_rootrick_actor_bind_clip(&e->actor,value);
        break;
    case BFM_ROOTRICK_SET_YAW:
        bfm_woods_rootrick_actor_set_yaw(&e->actor,value);
        break;
    case BFM_ROOTRICK_TURN_TO_PLAYER:
        bfm_woods_rootrick_actor_turn_to(&e->actor,hero->root.xyz);
        break;
    case BFM_ROOTRICK_ACTOR_MESSAGE:
    case BFM_ROOTRICK_PLAYER_MESSAGE:
        if (e->window || *update->opened) return 0;
        *update->opened = bfm_field_message_borrow(cmd->data,cmd->bytes);
        if (!*update->opened) return 0;
        e->window_offset = cmd->window_offset;
        e->window_right = (cmd->window_flags & 8u) != 0;
        e->window = 1;
        break;
    case BFM_ROOTRICK_RACE_CAMERA: {
        BfmVillageCamera presented = *camera;
        bfm_woods_rootrick_release_camera(e->blocking);
        if (!bfm_woods_rootrick_race_camera_begin(e->blocking,hero->root.xyz,&presented)) return 0;
        bfm_woods_box_view_present(update->view,&presented);
        e->fixed_shot = 0;
        e->race_camera = 1;
        break;
    }
    default:
        return 0;
    }
    return 1;
}

static int stage_cast(BfmWoodsRootrickEncounter *e, BfmVillageMotion *motion)
{
    if (e->staging.phase == BFM_ROOTRICK_LOAD_CAST && !e->art) {
        e->art = bfm_woods_rootrick_art_open(e->disc,e->scene,e->bytes);
        if (!e->art) return 0;
    }
    BfmRootrickReadiness ready = e->art ? BFM_ROOTRICK_RESOURCE_READY : BFM_ROOTRICK_RESOURCE_PENDING;
    switch (bfm_woods_rootrick_stage_step(&e->staging,ready,ready)) {
    case BFM_ROOTRICK_RESOURCE_ERROR: return 0;
    case BFM_ROOTRICK_FADE_SCORE: e->score = 0; break;
    case BFM_ROOTRICK_SHOW_IDLE:
        if (!bfm_woods_rootrick_actor_begin(&e->actor,bfm_village_motion_ground(motion),
                                           e->scene,e->bytes,e->staging.actor_xyz)) return 0;
        e->shadow = e->visible = 1;
        break;
    case BFM_ROOTRICK_START_OPENING:
        if (!bfm_woods_rootrick_challenge_begin(&e->script,e->scene,e->bytes)) return 0;
        e->phase = CHALLENGE;
        break;
    case BFM_ROOTRICK_NO_CHANGE: break;
    }
    return 1;
}

int bfm_woods_rootrick_update_ledges(BfmWoodsRootrickEncounter *e, BfmGameplayRun *run)
{
    return e && bfm_rootrick_wall_advance_props(e->wall, run);
}

static int step_wall(BfmWoodsRootrickEncounter *e, BfmVillageMotion *motion, BfmGameplayRun *run,
                     BfmFieldInput input, int viewing_yaw, unsigned actor_tick, int rival_arrived,
                     BfmForestMessage **opened)
{
    if (!bfm_rootrick_wall_step(e->wall, motion, &e->actor, rival_arrived, run, input, viewing_yaw,
                               actor_tick, &e->outcome)) return 0;
    const BfmRootrickWallEffects *events = bfm_rootrick_wall_effects(e->wall);
    if (!add_sound(e,events->hand_sound) || !add_sound(e,events->rival_sound)) return 0;
    if (events->rival_sound) bfm_woods_rootrick_impact_shake(&e->impact);
    if (events->retry_message) {
        const uint32_t base = UINT32_C(0x80128158);
        const uint32_t end = UINT32_C(0x80196550);
        if (*opened || e->window || events->retry_message < base || events->retry_message >= end ||
            end - base > e->bytes) return 0;
        *opened = bfm_field_message_borrow(e->scene + (events->retry_message - base),
                                           end - events->retry_message);
        if (!*opened) return 0;
        e->window = e->retry_window = 1;
    }
    BfmForestTransform hero, rival;
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    if (!bfm_bedroom_figure_actor_root(&p->root, p->pose.pitch, &hero)) return 0;
    bfm_forest_actor_pitch_yaw((unsigned)e->actor.pitch, e->actor.yaw, e->actor.xyz, &rival);
    const int16_t *hand = events->attached || events->hand_contact ? events->hand_xyz : NULL;
    if (!bfm_woods_rootrick_wall_art_step(&e->chips, run, hand, &hero, events->rival_dust_xyz,
                                         events->rival_dust_sites, &rival)) return 0;
    if (e->outcome != BFM_ROOTRICK_UNDECIDED) e->phase = CREST;
    return 1;
}

int bfm_woods_rootrick_step(BfmWoodsRootrickEncounter *e, BfmVillageMotion *motion,
                           BfmWoodsBoxView *view, BfmGameplayRun *run, BfmSaveRecord *record,
                           BfmFieldInput input, unsigned actor_tick, int hero_standing, BfmForestMessage **opened)
{
    if (!e || !motion || !view || !run || !record || !opened) return 0;
    const BfmVillagePlayer *hero = bfm_village_motion_player(motion);
    e->sounds_taken = e->sounds_issued = 0;
    if (e->phase == FINISHED) {
        if (e->return_camera) {
            BfmVillageCamera room = *bfm_woods_box_view_camera(view);
            e->return_camera = bfm_woods_rootrick_room_camera_step(e->blocking, &room);
            bfm_woods_box_view_present(view, &room);
        }
        return 1;
    }
    if (e->phase == FIELD) {
        if (!bfm_woods_rootrick_trigger_step(&e->trigger, run, hero->root.xyz)) return 1;
        e->blocking = bfm_woods_rootrick_blocking_open(e->scene,e->bytes,&hero->root);
        if (!e->blocking || !bfm_woods_rootrick_stage_begin(&e->staging,&e->trigger)) return 0;
        bfm_rootrick_wall_challenge(e->wall);
        e->phase = STAGING;
        return 1;
    }
    if (bfm_woods_rootrick_score_due(&e->staging.score)) e->score = e->staging.score.selector;
    BfmVillageCamera camera = *bfm_woods_box_view_camera(view);
    if (e->phase == STAGING || e->phase == CHALLENGE) {
        if (!service_player(e,&camera) ||
            !bfm_village_motion_staged(motion,bfm_woods_rootrick_player(e->blocking))) return 0;
        hero = bfm_village_motion_player(motion);
    }
    if (e->phase == STAGING && !stage_cast(e,motion)) return 0;
    int events = 0;
    int was_racing = e->phase == RACING;
    int was_crest = e->phase == CREST;
    if (e->visible && (e->phase == CHALLENGE || e->phase == TO_WALL ||
                       (was_racing && !e->rival_attached))) {
        events = bfm_woods_rootrick_actor_step(&e->actor,
                                  e->staging.score.state == BFM_ROOTRICK_SCORE_REQUESTED);
        if (events == BFM_ROOTRICK_ACTOR_ERROR) return 0;
        if (events & BFM_ROOTRICK_ACTOR_IMPACT) {
            bfm_woods_rootrick_impact_hit(&e->impact, e->actor.xyz);
            if (!add_sound(e, BFM_ROOTRICK_IMPACT_SOUND)) return 0;
        }
    }
    if (events & BFM_ROOTRICK_ACTOR_START_WALL) {
        if (e->phase != TO_WALL ||
            !bfm_rootrick_wall_begin(e->wall, motion, bfm_woods_rootrick_player_position(e->blocking)))
            return 0;
        e->phase = RACING;
    }
    if (events & BFM_ROOTRICK_ACTOR_AT_WALL) e->rival_attached = 1;
    if (was_racing) {
        int yaw = bfm_village_motion_view_yaw(camera.eye, camera.target);
        if (!step_wall(e, motion, run, input, yaw, actor_tick, events & BFM_ROOTRICK_ACTOR_AT_WALL,
                       opened)) return 0;
        hero = bfm_village_motion_player(motion);
    }
    int was_race_camera = e->race_camera;
    if (was_crest) {
        bfm_rootrick_wall_hold(e->wall, 1);
        e->summit = bfm_woods_rootrick_summit_open(e->disc, e->scene, e->bytes, e->main,
            e->main_bytes, motion, &e->actor, e->visible, e->outcome);
        if (!e->summit ||
            !bfm_woods_rootrick_outcome_begin(&e->script, e->scene, e->bytes, e->outcome)) return 0;
        e->phase = OUTCOME;
        if (e->outcome == BFM_ROOTRICK_LOSS) e->suppress_hurt = 1;
        if (e->retry_window) {
            e->window = e->retry_window = 0;
            e->retire_retry = 1;
        }
    }
    if (e->phase == OUTCOME) {
        BfmWoodsSummitUpdate field = {0};
        if (hero_standing < 0 && e->outcome == BFM_ROOTRICK_LOSS)
            hero_standing = bfm_rootrick_wall_standing(e->wall);
        if (!was_crest && !bfm_woods_rootrick_summit_step(e->summit, run,
                bfm_village_motion_view_yaw(camera.eye, camera.target), hero_standing, &field)) return 0;
        if (!bfm_woods_rootrick_wall_art_step(&e->chips, run, NULL, NULL, NULL, 0, NULL)) return 0;
        if (field.shake) bfm_woods_rootrick_impact_shake(&e->impact);
        if (field.voice) e->voice = field.voice;
        if (!add_sound(e, field.sound)) return 0;
        if (field.remove_shadow) e->shadow = 0;
        if (field.struck_hero) {
            e->pending_hit = 1;
            e->player_hit = field.impact;
        }
        SceneUpdate update = {e,view,motion,opened};
        int ready = e->room_score == RESTORE_INSTALLED;
        BfmRootrickResult result = bfm_woods_rootrick_outcome_step(&e->script, e->window,
                                field.release, ready, record->flags, script_command, &update);
        if (result == BFM_ROOTRICK_ERROR) return 0;
        /* Winning paths ask before the last speech. A loss asks when the
         * result actor retires. Wait for the room mixer to install the score. */
        if (e->room_score == RESTORE_IDLE &&
            (e->script.waiting_score || (e->outcome == BFM_ROOTRICK_LOSS && e->script.terminal))) {
            e->room_score = RESTORE_REQUESTED;
            e->staging.score = (BfmRootrickScore){0};
            e->score = TWINPEAK_SCORE;
        }
        e->visible = bfm_woods_rootrick_summit_visible(e->summit);
        if (result == BFM_ROOTRICK_FINISHED && bfm_woods_rootrick_outcome_can_retire(&e->script, ready)) {
            if (!bfm_woods_rootrick_summit_finish(e->summit, e->outcome == BFM_ROOTRICK_LOSS)) return 0;
            bfm_rootrick_wall_hold(e->wall, 0);
            e->visible = e->shadow = 0;
            e->phase = FINISHED;
        }
        hero = bfm_village_motion_player(motion);
    }
    if (e->phase == CHALLENGE) {
        SceneUpdate update = {e,view,motion,opened};
        BfmRootrickResult result = bfm_woods_rootrick_challenge_step(&e->script, e->window,
                                     events & BFM_ROOTRICK_ACTOR_RELEASE, record->flags,
                                     script_command, &update);
        if (result == BFM_ROOTRICK_ERROR) return 0;
        if (result == BFM_ROOTRICK_FINISHED) {
            bfm_woods_rootrick_score_arm(&e->staging.score,BFM_ROOTRICK_RACE_SCORE);
            e->score = 0;
            bfm_woods_rootrick_actor_begin_wall(&e->actor);
            e->phase = TO_WALL;
        }
    }
    /* The two actor scripts can change camera service during this update. */
    camera = *bfm_woods_box_view_camera(view);
    if (e->race_camera) {
        if (was_race_camera && !bfm_woods_rootrick_race_camera_step(e->blocking,hero->root.xyz,&camera))
            return 0;
    } else if (e->fixed_shot) {
        bfm_woods_rootrick_step_camera(e->blocking,&camera);
    } else {
        bfm_woods_box_view_follow(view, hero->root.xyz, e->restart_follow, 0);
        e->restart_follow = 0;
        camera = *bfm_woods_box_view_camera(view);
        if (e->return_camera)
            e->return_camera = bfm_woods_rootrick_room_camera_step(e->blocking, &camera);
    }
    bfm_woods_box_view_present(view,&camera);
    bfm_woods_rootrick_impact_tick(&e->impact);
    bfm_woods_box_view_shake(view, bfm_woods_rootrick_impact_camera_y(&e->impact));
    return 1;
}

void bfm_woods_rootrick_score_applied(BfmWoodsRootrickEncounter *e, unsigned selector)
{
    if (!e) return;
    bfm_woods_rootrick_score_acknowledge(&e->staging.score,selector);
    if (e->room_score == RESTORE_REQUESTED && selector == TWINPEAK_SCORE)
        e->room_score = RESTORE_INSTALLED;
}

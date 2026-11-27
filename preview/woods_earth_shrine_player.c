/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_player.h"

#include "bedroom_effects.h"
#include "field_waypoint.h"
#include "forest_pose.h"
#include "woods_earth_award_effect.h"
#include "woods_earth_player_award.h"
#include "woods_twinpeak_stride.h"

#include <stdlib.h>

struct BfmEarthShrinePlayer {
    const MusashiDiscMedia *disc;
    const uint8_t *scene;
    size_t bytes;
    BfmWoodsEarthPlayerAward *award;
    BfmWoodsEarthAwardEffect *effect;
    BfmEarthPlayerAwardFrame frame;
    int registered, effect_started, waiting_for_rays, window_open, window_closed;
    BfmForestMessage *pending;
    BfmEarthPlayerPresentation presentation;
    BfmEarthAwardEffectFrame effect_frame;
    unsigned body_sound, body_stop, next_effect_sound;
};

BfmEarthShrinePlayer *bfm_earth_shrine_player_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes)
{
    if (!disc || !scene || !bytes) return NULL;
    BfmEarthShrinePlayer *p = calloc(1, sizeof *p);
    if (p) {
        p->disc = disc;
        p->scene = scene;
        p->bytes = bytes;
    }
    return p;
}

void bfm_earth_shrine_player_free(BfmEarthShrinePlayer *p)
{
    if (!p) return;
    bfm_forest_message_free(p->pending);
    bfm_woods_earth_award_effect_free(p->effect);
    bfm_woods_earth_player_award_free(p->award);
    free(p);
}

int bfm_earth_shrine_player_begin(BfmEarthShrinePlayer *p, uint32_t descriptor)
{
    if (!p || p->award) return 0;
    BfmWoodsEarthPlayerAward *award = bfm_woods_earth_player_award_open(
        p->disc, p->scene, p->bytes, descriptor);
    if (!award) return 0;
    p->effect = bfm_woods_earth_award_effect_open(p->disc, p->scene, p->bytes);
    if (!p->effect) {
        bfm_woods_earth_player_award_free(award);
        return 0;
    }
    p->award = award;
    return 1;
}

int bfm_earth_shrine_player_active(const BfmEarthShrinePlayer *p)
{
    return p && p->award && !p->frame.complete;
}

int bfm_earth_shrine_player_complete(const BfmEarthShrinePlayer *p)
{
    return p && p->frame.complete;
}

static int advance_player(BfmEarthShrinePlayer *p, BfmVillageMotion *motion,
    const BfmBedroomView *view, BfmGameplayRun *run, BfmSaveRecord *save, int rays_ready)
{
    BfmEarthPlayerAwardInput in = {
        .effect_message_ready = rays_ready,
        .window_closed = p->window_closed
    };
    /* The zero-distance staging order finishes before the first turn. The
     * pending turn is serviced by the following player update. */
    if (p->frame.face_camera) {
        BfmBedroomRoot root = bfm_village_motion_player(motion)->root;
        int goal = bfm_waypoint_heading((int64_t)root.xyz[0] - view->eye[0],
                                        (int64_t)root.xyz[2] - view->eye[2]);
        in.facing_camera_done = bfm_twinpeak_turn(&root, goal);
        if (!bfm_village_motion_staged(motion, &root)) return 0;
    }
    if (!bfm_woods_earth_player_award_step(p->award, in, save, run, &p->frame)) return 0;
    p->window_closed = 0;
    if (p->frame.face_changed && !bfm_village_motion_combat_face(motion, p->frame.face_slot))
        return 0;
    if (p->frame.message) {
        p->pending = bfm_field_message_borrow(p->frame.message, p->frame.message_bytes);
        if (!p->pending) return 0;
    }
    if (p->frame.enter_presentation) p->presentation = BFM_EARTH_PLAYER_PRESENTATION_ENTER;
    if (p->frame.restore_presentation) p->presentation = BFM_EARTH_PLAYER_PRESENTATION_RESTORE;
    p->body_sound = p->frame.sound;
    if (p->frame.stop_sound == BFM_EARTH_SCROLL_RINGS_SOUND)
        p->body_stop = BFM_EARTH_SCROLL_STOP_RINGS_SOUND;
    return 1;
}

static int step_effect(BfmEarthShrinePlayer *p, const BfmBedroomRoot *foot,
    const BfmBedroomView *view, BfmGameplayRun *run, uint16_t tiles[512][1024],
    int spawn, int finish)
{
    BfmForestTransform hero;
    bfm_forest_actor_root(foot->yaw, foot->xyz, &hero);
    return bfm_woods_earth_award_effect_step(p->effect, run, spawn, finish,
        &hero, view, tiles, &p->effect_frame);
}

int bfm_earth_shrine_player_step(BfmEarthShrinePlayer *p, BfmVillageMotion *motion,
    const BfmBedroomView *view, BfmGameplayRun *run, BfmSaveRecord *save,
    uint16_t tiles[512][1024], BfmForestMessage **opened)
{
    if (!p) return 0;
    if (!p->award) return 1;
    if (!bfm_village_motion_player(motion) || !view || !run || !save || !tiles || !opened)
        return 0;
    if (!p->registered) {
        /* The room dispatches its queued order after this field's player
         * update. The new player action first runs on the following field. */
        p->registered = 1;
        return 1;
    }

    p->body_sound = p->body_stop = p->next_effect_sound = 0;
    int waiting = p->waiting_for_rays;
    if (waiting) {
        /* The waiting player does no work. The effect actor opens the window
         * on the field its callback fires, before another player tick. */
        if (!step_effect(p, &bfm_village_motion_player(motion)->root, view, run, tiles, 0, 0))
            return 0;
    }
    if (!p->frame.complete &&
        !advance_player(p, motion, view, run, save, waiting && p->effect_frame.message_ready))
        return 0;
    if (p->frame.message) p->waiting_for_rays = 0;
    if (p->pending && !*opened) {
        *opened = p->pending;
        p->pending = NULL;
        p->window_open = 1;
    }

    if (p->frame.spawn_earth_effect) {
        p->effect_started = 1;
        p->waiting_for_rays = 1;
    }
    if (p->effect_started && !waiting)
        return step_effect(p, &bfm_village_motion_player(motion)->root, view, run, tiles,
            p->frame.spawn_earth_effect, p->frame.finish_earth_effect);
    return 1;
}

int bfm_earth_shrine_player_closed(BfmEarthShrinePlayer *p)
{
    if (!p || !p->window_open) return 0;
    p->window_open = 0;
    p->window_closed = 1;
    return 1;
}

void bfm_earth_shrine_player_pose(const BfmEarthShrinePlayer *p, BfmBedroomPose *pose)
{
    if (!bfm_earth_shrine_player_active(p) || !p->frame.pose || !pose) return;
    pose->pose = p->frame.pose;
    pose->show_weapon = p->frame.show_fusion;
    pose->show_lumina = p->frame.show_lumina;
    pose->draw_current = 1;
}

int bfm_earth_shrine_player_draw(const BfmEarthShrinePlayer *p, BfmBedroomFigure *figure,
    const BfmBedroomView *view)
{
    return p && (!p->effect_started || bfm_woods_earth_award_effect_add(p->effect, figure, view));
}

unsigned bfm_earth_shrine_player_take_sound(BfmEarthShrinePlayer *p)
{
    if (!p) return 0;
    unsigned cue = p->body_sound;
    if (cue) p->body_sound = 0;
    else if ((cue = p->body_stop)) p->body_stop = 0;
    else if (p->next_effect_sound < p->effect_frame.sound_count)
        cue = p->effect_frame.sounds[p->next_effect_sound++];
    return cue;
}

BfmEarthPlayerPresentation bfm_earth_shrine_player_take_presentation(BfmEarthShrinePlayer *p)
{
    if (!p) return BFM_EARTH_PLAYER_PRESENTATION_NONE;
    BfmEarthPlayerPresentation event = p->presentation;
    p->presentation = BFM_EARTH_PLAYER_PRESENTATION_NONE;
    return event;
}

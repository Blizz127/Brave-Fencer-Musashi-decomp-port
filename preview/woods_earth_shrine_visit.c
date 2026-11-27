/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_visit.h"
#include "woods_earth_scroll_touch.h"
#include "woods_earth_shrine_field.h"
#include "woods_earth_shrine_hit.h"
#include "woods_earth_shrine_player.h"
#include "woods_earth_shrine_presentation.h"
#include "woods_earth_shrine_scene_art.h"
#include "woods_earth_shrine_tiles.h"
#include <stdlib.h>

struct BfmEarthShrineVisit {
    const MusashiDiscMedia *disc;
    const uint8_t *scene;
    size_t scene_bytes;
    uint16_t (*tiles)[1024];
    BfmVillageMotion *motion;
    BfmWoodsBoxView *view;
    BfmSaveRecord *save;
    BfmEarthShrineRelease child;
    BfmWoodsEarthShrineHit hit;
    BfmWoodsEarthScrollTouch touch;
    BfmEarthShrineField *field;
    BfmWoodsEarthShrineSceneArt *art;
    BfmEarthShrinePlayer *award;
    BfmEarthShrinePresentation presentation;
    BfmForestTransform previous_hero;
    unsigned member, score, sounds[12], sound_count, sound_read;
};

typedef struct {
    BfmEarthShrineVisit *visit;
    BfmGameplayRun *run;
    BfmForestMessage **opened;
    int failed;
} EarthUpdate;

void bfm_earth_visit_free(BfmEarthShrineVisit *v)
{
    if (!v) return;
    bfm_earth_presentation_clear(&v->presentation);
    bfm_earth_shrine_player_free(v->award);
    bfm_earth_shrine_field_free(v->field);
    bfm_woods_earth_shrine_scene_art_free(v->art);
    free(v);
}

BfmEarthShrineVisit *bfm_earth_visit_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
    BfmVillageMotion *motion, BfmWoodsBoxView *view, BfmSaveRecord *save)
{
    BfmEarthShrineVisit *v = calloc(1, sizeof *v);
    if (!v) return NULL;
    v->disc = disc;
    v->scene = scene;
    v->scene_bytes = bytes;
    v->tiles = tiles;
    v->motion = motion;
    v->view = view;
    v->save = save;
    v->field = bfm_earth_shrine_field_open(scene, bytes, main, main_bytes);
    v->art = bfm_woods_earth_shrine_scene_art_open(scene, bytes);
    v->award = bfm_earth_shrine_player_open(disc, scene, bytes);
    if (!v->award || !v->field || !v->art || !bfm_woods_earth_shrine_scene_art_bind_smoke(v->art, main, main_bytes) ||
        !bfm_woods_earth_shrine_hit_mount(scene, bytes, &v->hit) ||
        !bfm_woods_earth_scroll_touch_mount(scene, bytes, &v->touch)) {
        bfm_earth_visit_free(v);
        return NULL;
    }
    return v;
}

static int play(BfmEarthShrineVisit *v, unsigned cue)
{
    if (v->sound_count == sizeof v->sounds / sizeof v->sounds[0]) return 0;
    v->sounds[v->sound_count++] = cue;
    return 1;
}

static int host(void *context, const BfmEarthShrineRelease *child, BfmEarthReleaseEvent event,
                unsigned kind, uint32_t operand, const uint8_t *data, size_t bytes)
{
    EarthUpdate *u = context;
    BfmEarthShrineVisit *v = u->visit;
    int ok;
    switch (event) {
    case BFM_EARTH_CREST_STAGE: case BFM_EARTH_SHATTER: case BFM_EARTH_DUST: case BFM_EARTH_TRAIL:
        ok = bfm_woods_earth_shrine_scene_art_notify(v->art, child, event, kind, u->run,
                                                     bfm_woods_box_view_transform(v->view));
        /* Particles can exhaust their pools. The main shatter creates the
         * tablet fragments and must succeed before this field is accepted. */
        if (event == BFM_EARTH_SHATTER && !ok) u->failed = 1;
        return ok;
    case BFM_EARTH_TOUCH_SCROLL:
        return bfm_woods_earth_scroll_touch_test(&v->touch,
                bfm_woods_earth_shrine_scene_art_scroll_model(v->art), &v->previous_hero,
                BFM_EARTH_TOUCH_ORDINARY);
    case BFM_EARTH_PLAYER_READY: case BFM_EARTH_MESSAGE_CLOSED:
    case BFM_EARTH_CAMERA_READY: case BFM_EARTH_FIELD_COMMAND:
        return bfm_earth_shrine_field_event(v->field, child, event, kind, operand, data, bytes,
                                            v->view, u->opened);
    case BFM_EARTH_SOUND: ok = play(v, kind); break;
    case BFM_EARTH_SCORE: v->score = operand; return 1;
    case BFM_EARTH_SPECIAL_PALETTE: ok = bfm_woods_box_view_shrine_palette(v->view, kind); break;
    case BFM_EARTH_RELEASE_SHAKE:
        ok = bfm_earth_shrine_field_shake(v->field, data, bytes);
        break;
    case BFM_EARTH_LOAD_RELEASE:
        ok = bfm_woods_earth_shrine_tiles(v->disc, BFM_WOODS_EARTH_RELEASE_TILES, v->tiles, &v->member);
        break;
    case BFM_EARTH_BEFORE_AWARD:
        return bfm_earth_presentation_request(&v->presentation, BFM_WOODS_EARTH_AWARD_TILES);
    case BFM_EARTH_AFTER_AWARD:
        return bfm_earth_shrine_player_complete(v->award) &&
               bfm_earth_presentation_request(&v->presentation, BFM_WOODS_EARTH_RESTORE_TILES);
    case BFM_EARTH_LOAD_RETURN: return bfm_earth_presentation_wait_return(&v->presentation);
    case BFM_EARTH_AWARD_ACTION:
        if (v->member != BFM_WOODS_EARTH_AWARD_TILES) return 0;
        ok = bfm_earth_shrine_player_begin(v->award, operand);
        break;
    case BFM_EARTH_AWARD_CONFIRMED: return bfm_earth_shrine_player_complete(v->award);
    default: return 0;
    }
    if (!ok) u->failed = 1;
    return ok;
}

static void begin_field(BfmEarthShrineVisit *v, unsigned clock)
{
    v->sound_read = v->sound_count = 0;
    bfm_woods_earth_shrine_scene_art_set_field(v->art, clock, bfm_woods_box_view_third_lamp(v->view));
}

int bfm_earth_visit_strike(BfmEarthShrineVisit *v, BfmGameplayRun *run, unsigned clock,
                           const BfmWoodsCasterMeleeFrame *swing, BfmForestMessage **opened)
{
    if (!v || v->child.stage != BFM_EARTH_SEALED) return 1;
    begin_field(v, clock);
    BfmEarthShrineHitResult hit = bfm_woods_earth_shrine_hit_step(&v->hit,
                                  bfm_village_motion_ground(v->motion), swing);
    if (hit == BFM_EARTH_SHRINE_MISS) return 1;
    if (hit == BFM_EARTH_SHRINE_DEFLECTED) return play(v, 1062);
    int16_t xyz[3];
    for (unsigned i = 0; i < 3; ++i) xyz[i] = (int16_t)v->hit.root.translation[i];
    EarthUpdate u = {.visit=v, .run=run, .opened=opened};
    return bfm_earth_shrine_release_begin(&v->child, v->scene, v->scene_bytes, xyz, host, &u) && !u.failed;
}

int bfm_earth_visit_controls(const BfmEarthShrineVisit *v)
{
    return v && v->child.stage > BFM_EARTH_SEALED && v->child.stage < BFM_EARTH_RELEASED;
}

int bfm_earth_visit_step(BfmEarthShrineVisit *v, BfmGameplayRun *run, unsigned clock,
                         int blade_settled, BfmForestMessage **opened)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(v->motion);
    if (!p || !bfm_bedroom_figure_actor_root(&p->root, p->pose.pitch, &v->previous_hero)) return 0;
    begin_field(v, clock);
    if (!bfm_earth_presentation_step(&v->presentation, v->disc, v->tiles, &v->member)) return 0;
    if (!bfm_earth_shrine_player_step(v->award, v->motion, bfm_woods_box_view_transform(v->view),
                                      run, v->save, v->tiles, opened)) return 0;
    BfmEarthPlayerPresentation mode = bfm_earth_shrine_player_take_presentation(v->award);
    if (mode != BFM_EARTH_PLAYER_PRESENTATION_NONE)
        bfm_earth_presentation_mode(&v->presentation, mode == BFM_EARTH_PLAYER_PRESENTATION_ENTER);
    int ready = blade_settled && !bfm_earth_shrine_player_active(v->award) &&
                bfm_earth_presentation_player_ready(&v->presentation);
    if (!bfm_earth_shrine_field_player(v->field, v->motion, v->view, run, ready)) return 0;
    p = bfm_village_motion_player(v->motion);
    EarthUpdate u = {.visit=v, .run=run, .opened=opened};
    if (!bfm_earth_shrine_release_step(&v->child, p->root.xyz, host, &u) || u.failed) return 0;
    bfm_woods_earth_shrine_scene_art_update(v->art, &v->child);
    bfm_woods_box_view_light(v->view, v->save->progress[0x37]);
    bfm_woods_box_view_follow(v->view, p->root.xyz, 0, 0);
    return v->child.stage != BFM_EARTH_RELEASED || bfm_earth_shrine_field_finish(v->field, v->motion);
}

void bfm_earth_visit_camera(BfmEarthShrineVisit *v)
{
    if (v) bfm_earth_shrine_field_camera(v->field, v->view);
}

int bfm_earth_visit_close(BfmEarthShrineVisit *v)
{
    return v && (bfm_earth_shrine_player_closed(v->award) || bfm_earth_shrine_field_closed(v->field));
}

unsigned bfm_earth_visit_score(const BfmEarthShrineVisit *v, unsigned ordinary)
{
    return v && v->score ? v->score : ordinary;
}

unsigned bfm_earth_visit_sound(BfmEarthShrineVisit *v)
{
    if (!v) return 0;
    if (v->sound_read < v->sound_count) return v->sounds[v->sound_read++];
    return bfm_earth_shrine_player_take_sound(v->award);
}

int bfm_earth_visit_hud(BfmEarthShrineVisit *v)
{
    return v ? bfm_earth_shrine_field_take_hud(v->field) : 0;
}

void bfm_earth_visit_pose(const BfmEarthShrineVisit *v, BfmBedroomPose *pose)
{
    if (v) bfm_earth_shrine_player_pose(v->award, pose);
}

int bfm_earth_visit_dark(const BfmEarthShrineVisit *v)
{
    return v && bfm_earth_presentation_dark(&v->presentation);
}

int bfm_earth_visit_draw(const BfmEarthShrineVisit *v, unsigned age,
                         BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!v) return 1;
    if (!bfm_earth_visit_dark(v) &&
        (!bfm_woods_earth_shrine_scene_art_draw(v->art, &v->child, age, v->member, figure, view) ||
         !bfm_earth_shrine_field_draw_notice(v->field, figure, view))) return 0;
    return bfm_earth_shrine_player_draw(v->award, figure, view);
}

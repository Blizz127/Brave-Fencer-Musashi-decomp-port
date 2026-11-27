/* SPDX-License-Identifier: MIT */
#include "woods_shrink_cast.h"
#include "woods_shrink_motion.h"
#include "woods_shrink_shot.h"
#include "woods_shrink_ribbon.h"
#include "woods_shrink_light.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { SHRINK = 13, RELEASE_HOLD = 16, CAST_SOUND = 0x479 };
struct BfmWoodsShrinkCast {
    BfmWoodsShrinkMotion *motion;
    BfmWoodsShrinkShot shot;
    BfmWoodsShrinkLight muzzle;
    BfmForestDust atlas;
    BfmWoodsShrinkClip clip;
    unsigned tick, cost;
    int active, restore_face;
};

BfmWoodsShrinkCast *bfm_woods_shrink_cast_open(const MusashiDiscMedia *disc,
                                               const uint8_t *scene, size_t bytes)
{
    const size_t base = UINT32_C(0x80128158), room = UINT32_C(0x8018ea38) - base;
    if (!disc || !scene || bytes < room + 0x54 ||
        u32le(scene + (UINT32_C(0x801877c4) - base)) != room + base) return NULL;
    unsigned cost = scene[UINT32_C(0x8018a4f0) - base + SHRINK];
    if (!cost || (cost & 128u)) return NULL;
    BfmWoodsShrinkCast *cast = calloc(1, sizeof *cast);
    if (!cast) return NULL;
    cast->cost = cost;
    cast->motion = bfm_woods_shrink_motion_open(disc, u32le(scene + room + 0x4c),
                                                u32le(scene + room + 0x50));
    if (!cast->motion || !bfm_forest_dust_load(&cast->atlas, disc)) {
        bfm_woods_shrink_cast_free(cast);
        return NULL;
    }
    return cast;
}

void bfm_woods_shrink_cast_free(BfmWoodsShrinkCast *cast)
{
    if (cast) bfm_woods_shrink_motion_free(cast->motion);
    free(cast);
}

int bfm_woods_shrink_cast_claims(const BfmWoodsShrinkCast *cast, const BfmForestStatus *status,
                                 const BfmWoodsShrinkInput *input)
{
    if (!cast || !status || !input || input->interrupted) return 0;
    return cast->active || (input->can_begin && input->circle_pressed &&
                            (status->record[0x49] & 127u) == SHRINK);
}

static void begin(BfmWoodsShrinkCast *cast, BfmForestStatus *status, BfmWoodsShrinkFrame *frame)
{
    int available = bfm_forest_status_values(status).bp >= (int)cast->cost;
    status->record[0x49] = SHRINK;
    status->record[0x4d] = available ? 0 : 128;
    cast->clip = available ? BFM_SHRINK_WINDUP : BFM_SHRINK_EMPTY;
    cast->active = 1;
    cast->tick = 0;
    if (available) {
        bfm_forest_status_spend_bp(status, cast->cost);
        frame->paid = 1;
    } else {
        frame->refused = 1;
    }
}

static void finish(BfmWoodsShrinkCast *cast)
{
    cast->active = 0;
    cast->restore_face = 1;
}

int bfm_woods_shrink_cast_step(BfmWoodsShrinkCast *cast, BfmForestStatus *status,
                               const BfmBedroomRoot *root, const BfmWoodsShrinkInput *input,
                               BfmWoodsShrinkFrame *frame)
{
    if (!cast || !status || !root || !input || !input->eye || !frame) return 0;
    *frame = (BfmWoodsShrinkFrame){0};
    cast->restore_face = 0;
    if (!cast->active) {
        if (bfm_woods_shrink_cast_claims(cast, status, input)) begin(cast, status, frame);
    } else if (input->interrupted) {
        finish(cast);
    } else {
        ++cast->tick;
        if (cast->clip == BFM_SHRINK_RELEASE) {
            if (cast->tick > RELEASE_HOLD) finish(cast);
        } else if (cast->tick == bfm_woods_shrink_motion_count(cast->motion, cast->clip)) {
            if (cast->clip == BFM_SHRINK_EMPTY) {
                finish(cast);
            } else {
                /* A second paid cast replaces any ribbons still travelling
                 * from the previous one only when the new spell leaves. */
                BfmForestTransform transform;
                bfm_forest_actor_root(root->yaw, root->xyz, &transform);
                bfm_woods_shrink_shot_begin(&cast->shot, &transform);
                frame->sound = CAST_SOUND;
                cast->clip = BFM_SHRINK_RELEASE;
                cast->tick = 0;
            }
        }
    }
    if (cast->active && !cast->tick)
        frame->face_slot = bfm_woods_shrink_motion_face_slot(cast->motion, cast->clip);
    if (cast->restore_face && !input->interrupted) {
        frame->face_slot = bfm_woods_shrink_motion_face_slot(cast->motion, BFM_SHRINK_RECOVER_FACE);
        frame->finished = 1;
    }
    unsigned equipped = status->record[0x49] & 127u;
    if (!frame->sound)
        frame->target_contact = bfm_woods_shrink_shot_step(&cast->shot, equipped, input->target_root);
    bfm_woods_shrink_light_tick(&cast->muzzle, &cast->shot, input->eye,
                                input->field_tick, !!frame->sound, equipped);
    frame->movement_locked = cast->active;
    frame->player_lit = cast->active && cast->clip == BFM_SHRINK_RELEASE && cast->tick > 0;
    if (frame->player_lit) bfm_woods_shrink_player_lamps(input->field_tick, frame->lamps);
    return 1;
}

void bfm_woods_shrink_cast_pose(const BfmWoodsShrinkCast *cast, BfmBedroomPose *body)
{
    if (!cast || !body) return;
    if (cast->restore_face)
        body->face_word = bfm_woods_shrink_motion_face_word(cast->motion, BFM_SHRINK_RECOVER_FACE);
    if (!cast->active) return;
    unsigned key = cast->tick % bfm_woods_shrink_motion_count(cast->motion, cast->clip);
    body->pose = bfm_woods_shrink_motion_pose(cast->motion, cast->clip, key);
    body->face_word = bfm_woods_shrink_motion_face_word(cast->motion, cast->clip);
    body->show_lumina = 0;
    if (cast->clip != BFM_SHRINK_EMPTY) body->show_weapon = 0;
}

int bfm_woods_shrink_cast_face(const BfmWoodsShrinkCast *cast, uint16_t tiles[512][1024])
{
    if (!cast || !tiles) return 0;
    if (cast->active) return bfm_woods_shrink_motion_face(cast->motion, cast->clip, tiles);
    return !cast->restore_face || bfm_woods_shrink_motion_face(cast->motion, BFM_SHRINK_RECOVER_FACE, tiles);
}

int bfm_woods_shrink_cast_append(const BfmWoodsShrinkCast *cast, BfmBedroomFigure *figure,
                                 const BfmBedroomView *view)
{
    if (!cast || !figure || !view) return 0;
    if (view->focal <= 0 || view->focal > UINT16_MAX) return 0;
    BfmForestCamera camera = {.projection = (uint16_t)view->focal};
    for (unsigned i = 0; i < 3; ++i) {
        camera.translation[i] = view->translation[i];
        for (unsigned j = 0; j < 3; ++j) {
            if (view->axes[i][j] < INT16_MIN || view->axes[i][j] > INT16_MAX) return 0;
            camera.rotation[i][j] = (int16_t)view->axes[i][j];
        }
    }
    enum { CARDS = BFM_WOODS_SHRINK_RIBBON_CARDS + BFM_WOODS_SHRINK_MUZZLE_CARDS +
                   BFM_WOODS_SHRINK_EXPIRY_CARDS };
    BfmForestDustCard cards[CARDS];
    size_t count = bfm_woods_shrink_ribbon_project(&cast->shot, &camera, cards, CARDS);
    count += bfm_woods_shrink_light_project(&cast->muzzle, &camera, cards + count, CARDS - count);
    count += bfm_woods_shrink_shot_project_expiry(&cast->shot, &cast->atlas, &camera,
                                                  cards + count, CARDS - count);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}

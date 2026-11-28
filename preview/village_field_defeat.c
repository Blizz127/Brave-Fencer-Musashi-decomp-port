/* SPDX-License-Identifier: MIT */
#include "village_field_defeat.h"
#include "bedroom_score.h"
#include <limits.h>

static int draw_retry_actor(void *context, const BfmForestCamera *camera, uint32_t frame,
                            unsigned yaw, unsigned pitch, uint32_t visible,
                            const BfmForestModelLighting *lighting, uint32_t pixels[240][320])
{
    const BfmVillageFieldDefeat *actor = context;
    const uint32_t main_base = UINT32_C(0x800cedf8);
    if (frame < main_base || frame - main_base > actor->main_bytes ||
        actor->main_bytes - (frame - main_base) < 256) return 0;
    const BfmVillagePlayer *player = bfm_village_motion_player(actor->motion);
    BfmBedroomView view = {.focal = camera->projection};
    BfmBedroomBody body = {.pose = actor->main + (frame - main_base), .pose_bytes = 256,
        .heading = yaw, .pitch = pitch, .visible_parts = visible, .surface_normal = {0, -4096, 0},
        .face_word = player->pose.face_word, .blade_tick = *actor->age,
        .green_blade = (actor->save->flags[129 / 8] >> (129 % 8)) & 1u};
    for (unsigned i = 0; i < 3; ++i) {
        body.position[i] = player->root.xyz[i];
        view.translation[i] = camera->translation[i];
        for (unsigned j = 0; j < 3; ++j) view.axes[i][j] = camera->rotation[i][j];
    }
    if (visible & (1u << 22)) body.blade = &actor->field->fusion;
    if (visible & (1u << 25)) {
        body.lumina_guard = actor->field->guard;
        body.lumina_blade = actor->field->blade;
    }
    if (lighting) bfm_bedroom_figure_model_lighting(actor->field->hero, lighting);
    if (!bfm_bedroom_figure_prepare(actor->field->hero, &view, &body, actor->tiles,
                                    &pixels[0][0])) return 0;
    bfm_bedroom_figure_draw_to(actor->field->hero, INT_MIN);
    return 1;
}

BfmRetryScreen *bfm_village_field_defeat_open(BfmVillageFieldDefeat *actor,
    const MusashiDiscMedia *disc, const uint8_t *scene, size_t scene_bytes, size_t table_offset,
    const BfmVillageCamera *presented, BfmGameplayRun *run)
{
    if (!actor || !actor->field || !actor->field->field || !actor->field->hero ||
        !actor->motion || !actor->save || !actor->main || !actor->tiles || !actor->age ||
        !disc || !presented || !run || !scene || table_offset > scene_bytes ||
        scene_bytes - table_offset < BFM_RETRY_ROOM_DATA_BYTES) return NULL;
    const BfmVillagePlayer *player = bfm_village_motion_player(actor->motion);
    /* Start the collapse camera from the last presented field view. */
    if (!bfm_bedroom_field_set_focal(actor->field->field, presented->focal)) return NULL;
    bfm_bedroom_field_viewpoint(actor->field->field, presented->eye, presented->target, 1, 1);
    const BfmBedroomView *outgoing = bfm_bedroom_field_view(actor->field->field);
    BfmForestCamera camera = {.projection = (uint16_t)outgoing->focal};
    for (unsigned i = 0; i < 3; ++i) {
        camera.translation[i] = outgoing->translation[i];
        for (unsigned j = 0; j < 3; ++j) camera.rotation[i][j] = (int16_t)outgoing->axes[i][j];
    }
    /* The US field initializes its default wipe speed to slot two. The
     * outgoing death scene requests that default from the resident table. */
    uint8_t wipe_step;
    if (!bfm_bedroom_resident_data(disc, UINT32_C(0x80062ba0) + 2,
                                   &wipe_step, sizeof wipe_step) || !wipe_step) return NULL;
    const BfmWoodsRetryPresentation presentation = {
        .context = actor, .wipe_step = wipe_step, .draw_actor = draw_retry_actor,
        .world_cutoff = actor->world_cutoff
    };
    const BfmRetryMountedRoom room = {.data = scene + table_offset,
        .bytes = BFM_RETRY_ROOM_DATA_BYTES, .woods = &presentation};
    uint32_t visible = bfm_village_field_visible(&player->pose, actor->save->flags);
    return bfm_retry_screen_open_mounted(disc, &room, run, &camera, presented->target,
                                         player->root.xyz, player->root.yaw, visible);
}

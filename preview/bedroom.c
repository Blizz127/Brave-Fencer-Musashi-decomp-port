/* SPDX-License-Identifier: MIT */
#include "bedroom.h"
#include "bedroom_fade.h"
#include "bedroom_field.h"
#include "bedroom_figure.h"
#include "bedroom_motion.h"
#include "bedroom_script.h"
#include "bedroom_travel.h"
#include "scene_tiles.h"
#include <stdlib.h>
#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
struct BfmBedroom {
    uint8_t *scene, *main;
    size_t scene_bytes;
    uint16_t tiles[512][1024];
    BfmBedroomEntry entry;
    BfmBedroomScript script;
    BfmBedroomField *field;
    BfmBedroomMotion *motion;
    BfmBedroomFigure *figure;
    BfmBedroomTravel *travel;
    BfmBedroomFade fade;
    BfmBedroomAction requested, pending;
    BfmBedroomPose pose;
    int camera_actor[3];
    unsigned update, presented, drain;
    uint32_t frames[3][240][320];
};

static unsigned half(const uint8_t *p)
{
    return p[0] | ((unsigned)p[1] << 8);
}
static uint32_t word(const uint8_t *p)
{
    return half(p) | ((uint32_t)half(p + 2) << 16);
}
static const uint8_t *data_range(const BfmBedroom *room, uint32_t address, size_t size)
{
    if (address < scene_base || size > room->scene_bytes ||
        address - scene_base > room->scene_bytes - size) return NULL;
    return room->scene + (address - scene_base);
}
void bfm_bedroom_free(BfmBedroom *room)
{
    if (!room) return;
    bfm_bedroom_field_free(room->field);
    bfm_bedroom_motion_free(room->motion);
    bfm_bedroom_figure_free(room->figure);
    bfm_bedroom_travel_free(room->travel);
    free(room->main);
    free(room->scene);
    free(room);
}
BfmBedroom *bfm_bedroom_open(const MusashiDiscMedia *disc, const BfmSaveRecord *record)
{
    BfmBedroomEntry entry;
    if (!bfm_bedroom_entry(disc, record, &entry)) return NULL;
    BfmBedroom *room = calloc(1, sizeof *room);
    if (!room) return NULL;
    room->entry = entry;
    room->scene = malloc(BFM_FIELD_SCENE_CAPACITY);
    if (!room->scene) goto failed;
    room->scene_bytes = bfm_read_field_scene(disc, entry.scene_member, room->scene);
    const uint8_t *root = data_range(room, 0x80182364u, 4);
    if (!root) goto failed;
    uint32_t address = word(root);
    const uint8_t *description = data_range(room, address, 0xcc);
    if (!description) goto failed;
    const uint8_t *placement = data_range(room, word(description + 0xb4), 20);
    if (!placement || half(placement + 6) != 360) goto failed;
    for (unsigned axis = 0; axis < 3; ++axis)
        room->camera_actor[axis] = (int16_t)half(placement + 2 * axis);
    room->field = bfm_bedroom_field_load(room->scene, room->scene_bytes, address);
    size_t main_bytes;
    room->main = bfm_bedroom_player_data(disc, &main_bytes);
    if (!room->main || !room->field) goto failed;
    room->motion = bfm_bedroom_motion_open(room->scene, room->scene_bytes, room->main, main_bytes);
    room->figure = bfm_bedroom_figure_load(disc, room->scene, room->scene_bytes, address);
    room->travel = bfm_bedroom_travel_open(room->scene, room->scene_bytes);
    if (!room->motion || !room->figure || !room->travel ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, room->tiles) ||
        !bfm_scene_read_tiles(disc, "SC01.CD", entry.scene_member, room->tiles) ||
        !bfm_bedroom_script_begin(&room->script, room->scene, room->scene_bytes)) goto failed;
    bfm_bedroom_fade_begin(&room->fade);
    /* The early-story actor starts its program with progress 10 and clears
     * four transient scene flags in the restored live record. */
    room->entry.state.variables[226] = 0;
    room->entry.state.flags[303 / 8] |= (uint8_t)(1u << (303 % 8));
    room->entry.state.progress[8] = 10;
    for (unsigned flag = 293; flag <= 296; ++flag)
        room->entry.state.flags[flag >> 3] &= (uint8_t)~(1u << (flag & 7));
    return room;
failed:
    bfm_bedroom_free(room);
    return NULL;
}

static int apply(void *context, BfmBedroomEvent event, uint32_t operand)
{
    BfmBedroom *room = context;
    switch (event) {
    case BFM_BED_CAMERA_CUT:
    case BFM_BED_CAMERA_MOVE:
        return bfm_bedroom_field_camera(room->field, room->scene, room->scene_bytes,
                         operand, room->camera_actor, event == BFM_BED_CAMERA_CUT);
    case BFM_BED_LIE: room->requested = BFM_BEDROOM_LIE; break;
    case BFM_BED_RISE: room->requested = BFM_BEDROOM_RISE; break;
    case BFM_BED_YAWN: room->requested = BFM_BEDROOM_YAWN; break;
    case BFM_BED_UPRIGHT: room->pending = BFM_BEDROOM_UPRIGHT; break;
    case BFM_BED_TURN: return bfm_bedroom_travel_turn(room->travel, operand & 4095u);
    case BFM_BED_WALK: return bfm_bedroom_travel_walk(room->travel, (int16_t)operand,
                                                      (int16_t)(operand >> 16));
    case BFM_BED_CUE:
    case BFM_BED_WINDOW_POSITION:
    case BFM_BED_WINDOW:
        break;
    }
    return 1;
}

static int render_scene(BfmBedroom *room, const BfmBedroomPose *pose, uint32_t pixels[240][320])
{
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x)
            pixels[y][x] = UINT32_C(0xff000000);
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(room->travel);
    const BfmBedroomView *view = bfm_bedroom_field_view(room->field);
    if (pose->pose) {
        BfmBedroomBody body = { .pose = pose->pose, .pose_bytes = 256,
            .heading = root->yaw, .pitch = pose->pitch, .shadow_visible = root->shadow_visible,
            .face_word = pose->face_word };
        /* The wake poses stow both weapons. Flag 128 owns the second sword. */
        body.visible_parts = UINT32_C(0x7fffffff) &
                             ~((UINT32_C(1) << 22) | (UINT32_C(1) << 25));
        if (!(room->entry.state.flags[128 / 8] & 1))
            body.visible_parts &= ~(UINT32_C(1) << 24);
        for (unsigned axis = 0; axis < 3; ++axis) {
            body.position[axis] = root->xyz[axis];
            body.surface_normal[axis] = root->normal[axis];
            body.shadow_position[axis] = root->shadow_xyz[axis];
            body.shadow_normal[axis] = root->shadow_normal[axis];
        }
        if (!bfm_bedroom_figure_prepare(room->figure, view, &body, room->tiles, &pixels[0][0])) return 0;
    }
    bfm_bedroom_field_draw(room->field, room->tiles, &pixels[0][0],
                   pose->pose ? bfm_bedroom_figure_draw_to : NULL, room->figure);
    bfm_bedroom_fade_draw(&room->fade, pixels);
    return 1;
}

int bfm_bedroom_step(BfmBedroom *room)
{
    if (room->script.reached_dialogue) {
        if (room->drain < 2) {
            room->presented = (room->presented + 1) % 3;
            ++room->drain;
        }
        return 1;
    }
    const BfmBedroomView *view = bfm_bedroom_field_view(room->field);
    if (room->update && !bfm_bedroom_travel_step(room->travel, view->eye, view->target))
        return 0;
    if (room->update) bfm_bedroom_field_step(room->field);
    if (room->update) bfm_bedroom_fade_step(&room->fade);
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(room->travel);
    BfmBedroomAction action = room->pending;
    switch (root->locomotion_edge) {
    case BFM_TRAVEL_WALK: action = BFM_BEDROOM_WALK; break;
    case BFM_TRAVEL_FALL: action = BFM_BEDROOM_FALL; break;
    case BFM_TRAVEL_LAND: action = BFM_BEDROOM_LAND; break;
    case BFM_TRAVEL_STAND: action = BFM_BEDROOM_STAND; break;
    default: break;
    }
    /* The submitted part transforms belong to the previous actor update. */
    BfmBedroomPose displayed = room->pose;
    if (displayed.pose && !bfm_bedroom_motion_face(room->motion, room->tiles)) return 0;
    if (action || room->pose.pose) {
        if (!bfm_bedroom_motion_step(room->motion, action, &room->pose)) return 0;
    }
    room->pending = room->requested;
    room->requested = BFM_BEDROOM_NONE;
    unsigned submitted = room->update % 3;
    if (!bfm_bedroom_script_tick(&room->script, !root->busy, apply, room) ||
        !render_scene(room, &displayed, room->frames[submitted])) return 0;
    /* Scene update, draw submission and scanout are separate game stages. */
    room->presented = (submitted + 1) % 3;
    ++room->update;
    return 1;
}

int bfm_bedroom_draw(BfmBedroom *room, uint32_t pixels[240][320])
{
    memcpy(pixels, room->frames[room->presented], sizeof room->frames[0]);
    return 1;
}

int bfm_bedroom_endpoint(const BfmBedroom *room)
{
    return room->script.reached_dialogue && room->drain == 2;
}

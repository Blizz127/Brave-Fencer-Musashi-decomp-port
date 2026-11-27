/* SPDX-License-Identifier: MIT */
#include "forest_maze_view.h"
#include "village_field.h"
#include "village_lighting.h"

#include <stdlib.h>

static const uint32_t scene_origin = UINT32_C(0x80128158);

struct BfmForestMazeView {
    BfmForestTerrain *terrain;
    BfmBedroomFigure *hero;
    BfmForestBlade fusion;
    const int8_t *guard, *blade;
    const uint8_t *scene;
    size_t scene_bytes;
    BfmVillageDaylight daylight;
    BfmVillageCamera camera;
    int16_t center[3], radii[2];
    unsigned zoom, field;
};

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t read32(const uint8_t *p)
{
    return read16(p) | (uint32_t)read16(p + 2) << 16;
}

static const uint8_t *mounted(const uint8_t *scene, size_t size,
                              uint32_t address, size_t length)
{
    if (address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= size && length <= size - offset ? scene + offset : NULL;
}

BfmForestMazeView *bfm_forest_maze_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t size, unsigned entrance)
{
    if (!disc || !scene || (entrance != 0 && entrance != 33)) return NULL;
    const uint8_t *header = mounted(scene, size, UINT32_C(0x8018738c), 4);
    if (!header) return NULL;
    const uint8_t *room = mounted(scene, size, read32(header), 0xcc);
    if (!room) return NULL;
    const uint8_t *mesh = mounted(scene, size, read32(room + 0x94), 8);
    const uint8_t *entrances = mounted(scene, size, read32(room + 0xc8), 4 * (entrance + 1));
    const uint8_t *target = mounted(scene, size, UINT32_C(0x8018bc18), 6);
    const uint8_t *radii = mounted(scene, size, UINT32_C(0x8018bc3c), 4);
    if (!mesh || read32(mesh) != 0x42 || !entrances || !target || !radii) return NULL;
    const uint8_t *door = mounted(scene, size, read32(entrances + 4 * entrance), 28);
    uint32_t destination = entrance ? UINT32_C(0x00023014) : UINT32_C(0x301b);
    if (!door || read32(door) != destination || read32(door + 24) != UINT32_C(0x8017dc04))
        return NULL;

    BfmForestMazeView *view = calloc(1, sizeof *view);
    if (!view) return NULL;
    view->terrain = bfm_forest_terrain_load_archive(disc, "SC03.CD", 7, mesh,
                                                    size - (size_t)(mesh - scene));
    const uint32_t face = UINT32_C(0x80189204);
    view->guard = (const int8_t *)mounted(scene, size, face + 0x12bc, 20);
    view->blade = (const int8_t *)mounted(scene, size, face + 0x12d0, 28);
    view->hero = bfm_bedroom_figure_load(disc, scene, size, read32(header), face, face + 0xd6c);
    view->scene = scene;
    view->scene_bytes = size;
    view->radii[0] = (int16_t)read16(radii);
    view->radii[1] = (int16_t)read16(radii + 2);
    if (!view->terrain || !view->hero || !view->guard || !view->blade ||
        !bfm_castle_blade_load(&view->fusion, scene, size, face - scene_origin + 0x1110) ||
        view->radii[0] <= 0 || view->radii[1] <= 0) {
        bfm_forest_maze_view_free(view);
        return NULL;
    }
    int16_t start[3];
    for (unsigned i = 0; i < 3; ++i) {
        view->center[i] = (int16_t)read16(target + 2*i);
        start[i] = (int16_t)read16(door + 16 + 2*i);
    }
    view->camera = (BfmVillageCamera){.focal = 500, .yaw = 2048, .following = 1,
                                      .radius = view->radii[0]};
    view->camera.target[1] = start[1];
    bfm_village_camera_lower(&view->camera, start, 0);
    return view;
}

void bfm_forest_maze_view_free(BfmForestMazeView *view)
{
    if (view) {
        bfm_forest_terrain_free(view->terrain);
        bfm_bedroom_figure_free(view->hero);
        free(view);
    }
}

static int16_t clamp_axis(int16_t coordinate, int16_t center)
{
    if (coordinate < center - 640) return (int16_t)(center - 640);
    if (coordinate > center + 640) return (int16_t)(center + 640);
    return coordinate;
}

void bfm_forest_maze_view_step(BfmForestMazeView *view, const int16_t xyz[3], int shoulder_pressed)
{
    if (!view || !xyz) return;
    if (shoulder_pressed) view->zoom ^= 1u;
    int delta = view->radii[view->zoom] - view->camera.radius;
    int step = delta / 4;
    if (!step && delta) step = delta > 0 ? 1 : -1;
    view->camera.radius += step;
    int16_t target[3] = { clamp_axis(xyz[0], view->center[0]), xyz[1],
                          clamp_axis(xyz[2], view->center[2]) };
    bfm_village_camera_lower(&view->camera, target, 0);
    ++view->field;
}

uint16_t (*bfm_forest_maze_view_tiles(BfmForestMazeView *view))[1024]
{
    return view ? bfm_forest_terrain_room_tiles(view->terrain) : NULL;
}

int bfm_forest_maze_view_daylight(BfmForestMazeView *view, unsigned phase)
{
    if (!view || phase >= 5) return 0;
    uint16_t (*tiles)[1024] = bfm_forest_terrain_room_tiles(view->terrain);
    if (view->daylight.scene) {
        bfm_village_daylight_step(&view->daylight, phase, tiles);
    } else if (!bfm_forest_maze_daylight_begin(&view->daylight, view->scene, view->scene_bytes,
                                               phase, tiles)) {
        return 0;
    }
    return bfm_bedroom_figure_lights(view->hero, view->daylight.lamps);
}

const BfmVillageCamera *bfm_forest_maze_view_camera(const BfmForestMazeView *view)
{
    return view ? &view->camera : NULL;
}

int bfm_forest_maze_view_draw_room(BfmForestMazeView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], const BfmForestActor *actors,
    const BfmForestMazeShadow *shadows, size_t actor_count, const BfmForestDustCard *cards,
    size_t card_count, uint32_t pixels[240][320])
{
    if (!view || !root || !pose || !pose->pose || !flags || !pixels ||
        actor_count > BFM_FOREST_ACTOR_LIMIT || (actor_count && !actors)) return 0;

    BfmForestCamera camera;
    int32_t near_target[3];
    for (unsigned i = 0; i < 3; ++i) {
        int eye = view->camera.eye[i];
        near_target[i] = eye + (view->camera.target[i] - eye) / 2;
    }
    if (!bfm_forest_camera_look_at(view->camera.eye, near_target,
                                   (uint16_t)view->camera.focal, &camera)) return 0;

    BfmBedroomView field = {.focal = view->camera.focal};
    BfmBedroomBody body = {
        .pose = pose->pose, .pose_bytes = 256, .heading = (int)root->yaw, .pitch = (int)pose->pitch,
        .face_word = pose->face_word, .shadow_visible = (int)root->shadow_visible,
        .visible_parts = bfm_village_field_visible(pose, flags), .blade_tick = view->field,
        .lumina_scale_q12 = pose->lumina_scale_q12, .green_blade = (flags[129 / 8] >> (129 % 8)) & 1u
    };
    for (unsigned i = 0; i < 3; ++i) {
        field.eye[i] = view->camera.eye[i];
        field.target[i] = view->camera.target[i];
        field.translation[i] = camera.translation[i];
        for (unsigned j = 0; j < 3; ++j) field.axes[i][j] = camera.rotation[i][j];
        body.position[i] = root->xyz[i];
        body.surface_normal[i] = root->normal[i];
        body.shadow_position[i] = root->shadow_xyz[i];
        body.shadow_normal[i] = root->shadow_normal[i];
    }
    if (body.visible_parts & (1u << 22)) body.blade = &view->fusion;
    if (body.visible_parts & (1u << 25)) {
        body.lumina_guard = view->guard;
        body.lumina_blade = view->blade;
    }
    if (!bfm_bedroom_figure_prepare(view->hero, &field, &body,
                     bfm_forest_terrain_room_tiles(view->terrain), &pixels[0][0])) return 0;
    for (size_t i = 0; i < actor_count; ++i) {
        const BfmForestMazeShadow *shadow = shadows ? &shadows[i] : NULL;
        if (!bfm_bedroom_figure_add_actor(view->hero, &field, &actors[i],
                shadow ? shadow->ground : NULL, shadow ? shadow->descriptor : NULL,
                shadow ? shadow->size : 0)) return 0;
    }
    if (!bfm_bedroom_figure_add_fusion(view->hero, &field, NULL, cards, card_count)) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    return bfm_forest_terrain_draw_field(view->terrain, &camera, view->field & 1u, pixels,
                                         bfm_bedroom_figure_draw_to, view->hero);
}

int bfm_forest_maze_view_draw_effects(BfmForestMazeView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *body, const uint8_t flags[64],
    const BfmForestActor *actors, size_t actor_count, const BfmForestDustCard *cards,
    size_t card_count, uint32_t pixels[240][320])
{
    return bfm_forest_maze_view_draw_room(view, root, body, flags, actors, NULL, actor_count,
                                          cards, card_count, pixels);
}

int bfm_forest_maze_view_draw(BfmForestMazeView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *body, const uint8_t flags[64],
    const BfmForestActor *actors, size_t actor_count, uint32_t pixels[240][320])
{
    return bfm_forest_maze_view_draw_effects(view, root, body, flags, actors, actor_count,
                                           NULL, 0, pixels);
}

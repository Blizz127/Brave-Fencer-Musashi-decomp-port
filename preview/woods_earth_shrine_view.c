/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_view.h"
#include "woods_capture_light.h"
#include "woods_fusion_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

static const uint32_t scene_origin = UINT32_C(0x80128158);
enum { MODEL_COUNT = 2, CREST_KEYS = 6, SCROLL_KEYS = 16 };

typedef struct {
    uint16_t page;
    uint8_t u, v;
    unsigned end;
} CrestKey;

typedef struct {
    uint16_t page, clut;
    uint8_t u, v, width, height;
    int16_t left, top;
    unsigned end;
} ScrollCell;

struct BfmWoodsEarthShrineView {
    MusashiForestPlayerMesh *mesh[MODEL_COUNT];
    BfmForestTransform root;
    int16_t crest_origin[3], crest_left, crest_top;
    uint8_t width, height;
    CrestKey keys[CREST_KEYS];
    ScrollCell detached_panel, scroll[SCROLL_KEYS];
};

static const uint8_t *region(const uint8_t *scene, size_t bytes, uint32_t address, size_t size)
{
    if (!scene || address < scene_origin || address - scene_origin > bytes ||
        size > bytes - (address - scene_origin)) return NULL;
    return scene + (address - scene_origin);
}

static const uint8_t *placement(const uint8_t *scene, size_t bytes)
{
    const uint8_t *room = region(scene, bytes, UINT32_C(0x801961e8), 0xbc);
    if (!room) return NULL;
    uint32_t begin = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= begin || (end - begin) % 20) return NULL;
    const uint8_t *rows = region(scene, bytes, begin, end - begin);
    if (!rows) return NULL;
    const uint8_t *result = NULL;
    for (size_t offset = 0; offset < end - begin; offset += 20) {
        const uint8_t *row = rows + offset;
        unsigned kind = u16le(row + 6);
        if (!kind) return result;
        if (kind == 268) {
            if (result || u16le(row + 8) || u16le(row + 12)) return NULL;
            result = row;
        }
    }
    return NULL;
}

static int load_crest(BfmWoodsEarthShrineView *shrine, const uint8_t *scene,
                      size_t bytes, const int16_t xyz[3])
{
    const uint8_t *sprite = region(scene, bytes, UINT32_C(0x8019595c), 12);
    const uint8_t *keys = region(scene, bytes, UINT32_C(0x80195968), 8 * CREST_KEYS);
    if (!sprite || !keys || !sprite[2] || !sprite[3]) return 0;
    shrine->width = sprite[2];
    shrine->height = sprite[3];
    unsigned total = 0;
    for (unsigned i = 0; i < CREST_KEYS; ++i) {
        const uint8_t *step = keys + i * 8;
        unsigned x = u16le(step), y = u16le(step + 2), hold = u32le(step + 4);
        if (x >= 1024 || y >= 512 || !(hold & 127) ||
            (hold >> 7) != (i + 1 == CREST_KEYS)) return 0;
        unsigned u = 2 * (x % 64), v = y % 256;
        if (u + shrine->width > 256 || v + shrine->height > 256) return 0;
        total += hold & 127;
        /* The field sprite is an additive raw-texture quad in an 8-bit page. */
        shrine->keys[i] = (CrestKey){
            .page = (uint16_t)(0xa0u | x / 64 | (y / 256) << 4),
            .u = (uint8_t)u, .v = (uint8_t)v, .end = total
        };
    }
    if (xyz[1] < INT16_MIN + 224 || xyz[2] < INT16_MIN + 16) return 0;
    shrine->crest_origin[0] = xyz[0];
    shrine->crest_origin[1] = (int16_t)(xyz[1] - 224);
    shrine->crest_origin[2] = (int16_t)(xyz[2] - 16);
    shrine->crest_left = s16le(sprite + 8);
    shrine->crest_top = s16le(sprite + 10);
    if (shrine->crest_left > INT16_MAX - shrine->width ||
        shrine->crest_top > INT16_MAX - shrine->height) return 0;
    return 1;
}

static int read_scroll_cell(ScrollCell *cell, const uint8_t *sprite)
{
    if (!sprite || sprite[0] != 255 || !sprite[2] || !sprite[3]) return 0;
    unsigned x = u16le(sprite + 4), y = u16le(sprite + 6);
    unsigned u = (x % 64) * 2, v = y % 256;
    if (x >= 1024 || y >= 512 || u + sprite[2] > 255 || v + sprite[3] > 255) return 0;
    cell->page = (uint16_t)(0xa0u | x / 64 | (y / 256) << 4);
    cell->clut = (uint16_t)(((256u + sprite[1]) << 6) | (256u / 16));
    cell->u = (uint8_t)u;
    cell->v = (uint8_t)v;
    cell->width = sprite[2];
    cell->height = sprite[3];
    cell->left = s16le(sprite + 8);
    cell->top = s16le(sprite + 10);
    return 1;
}

static int load_scroll(BfmWoodsEarthShrineView *shrine, const uint8_t *scene, size_t bytes)
{
    const uint8_t *sequence = region(scene, bytes, UINT32_C(0x80195a94), (SCROLL_KEYS + 1) * 8);
    if (!sequence || !read_scroll_cell(&shrine->detached_panel,
                                       region(scene, bytes, UINT32_C(0x801959c8), 12))) return 0;
    unsigned elapsed = 0;
    for (unsigned i = 0; i < SCROLL_KEYS; ++i) {
        const uint8_t *step = sequence + 8 * i;
        const uint8_t *sprite = region(scene, bytes, u32le(step), 12);
        unsigned hold = u32le(step + 4);
        if (!hold || hold > INT16_MAX || !read_scroll_cell(&shrine->scroll[i], sprite)) return 0;
        elapsed += hold;
        shrine->scroll[i].end = elapsed;
    }
    const uint8_t *stop = sequence + 8 * SCROLL_KEYS;
    return !u32le(stop) && u32le(stop + 4) == UINT16_C(0xfffe);
}

BfmWoodsEarthShrineView *bfm_woods_earth_shrine_view_open(const uint8_t *scene, size_t bytes)
{
    const uint8_t *row = placement(scene, bytes);
    if (!row) return NULL;
    BfmWoodsEarthShrineView *shrine = calloc(1, sizeof *shrine);
    if (!shrine) return NULL;
    int16_t xyz[3] = {s16le(row), s16le(row + 2), s16le(row + 4)};
    bfm_forest_actor_root(0, xyz, &shrine->root);
    const uint32_t models[MODEL_COUNT] = {UINT32_C(0x801e4d54), UINT32_C(0x801e5340)};
    for (unsigned i = 0; i < MODEL_COUNT; ++i) {
        unsigned parts;
        shrine->mesh[i] = musashi_forest_player_mesh_from_table(scene, bytes, scene_origin,
                                                                models[i], &parts);
        if (!shrine->mesh[i] || parts != 1) goto failed;
    }
    if (!load_crest(shrine, scene, bytes, xyz) || !load_scroll(shrine, scene, bytes)) goto failed;
    return shrine;
failed:
    bfm_woods_earth_shrine_view_free(shrine);
    return NULL;
}

void bfm_woods_earth_shrine_view_free(BfmWoodsEarthShrineView *shrine)
{
    if (!shrine) return;
    for (unsigned i = 0; i < MODEL_COUNT; ++i)
        musashi_forest_player_mesh_free(shrine->mesh[i]);
    free(shrine);
}

static int fixed_to_integer(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

int bfm_woods_earth_shrine_view_draw(const BfmWoodsEarthShrineView *shrine,
    BfmBedroomFigure *figure, const BfmBedroomView *view, int tablet_visible,
    unsigned animation_field)
{
    if (!shrine || !figure || !view) return 0;
    return !tablet_visible || bfm_woods_earth_shrine_view_attached(shrine, figure, view, 0,
                                                                 animation_field);
}

int bfm_woods_earth_shrine_view_attached(const BfmWoodsEarthShrineView *shrine,
    BfmBedroomFigure *figure, const BfmBedroomView *view, int tablet_dx, unsigned crest_age)
{
    if (!shrine || !figure || !view) return 0;
    BfmForestTransform root = shrine->root;
    root.translation[0] += tablet_dx;
    static const BfmForestPose upright = {
        .local = {{.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}}}
    };
    for (unsigned i = 0; i < MODEL_COUNT; ++i) {
        BfmForestActor actor = {
            .mesh = shrine->mesh[i], .pose = &upright, .root = &root, .visible_parts = 1
        };
        if (!bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0)) return 0;
    }
    unsigned tick = crest_age % shrine->keys[CREST_KEYS - 1].end;
    const CrestKey *key = shrine->keys;
    while (tick >= key->end) ++key;
    BfmForestDustCard card = {
        .material = {.page = key->page, .clut = (493u << 6) | (256u / 16),
                     .count = 4, .textured = 1, .translucent = 1}
    };

    /* Transform the sprite's local axes before projecting. Perspective across
     * this world-XY plane matters as Musashi approaches from one side. */
    BfmBedroomView sprite_view = *view;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t translated = 0;
        for (unsigned j = 0; j < 3; ++j)
            translated += (int64_t)view->axes[axis][j] * shrine->crest_origin[j];
        sprite_view.translation[axis] += fixed_to_integer(translated);
        sprite_view.axes[axis][0] = fixed_to_integer((int64_t)view->axes[axis][0] * 8704);
        sprite_view.axes[axis][1] = fixed_to_integer((int64_t)view->axes[axis][1] * 5632);
    }
    int depth_sum = 0;
    for (unsigned i = 0; i < 4; ++i) {
        int16_t corner[3] = {
            (int16_t)(shrine->crest_left + (i & 1 ? shrine->width : 0)),
            (int16_t)(shrine->crest_top + (i & 2 ? shrine->height : 0)), 0
        };
        int screen[3];
        if (!bfm_bedroom_project(&sprite_view, corner, screen)) return 1;
        depth_sum += screen[2];
        card.vertices[i] = (BfmForestRasterVertex){
            .x = screen[0], .y = screen[1], .u = (uint8_t)(key->u + (i & 1 ? shrine->width - 1 : 0)),
            .v = (uint8_t)(key->v + (i & 2 ? shrine->height - 1 : 0)), .rgb = 0x808080
        };
    }
    card.bucket = (unsigned)depth_sum / 16 + 1;
    return card.bucket >= 4096 || bfm_bedroom_figure_add_fusion(figure, view, NULL, &card, 1);
}

int bfm_woods_earth_shrine_view_scroll(const BfmWoodsEarthShrineView *shrine,
    BfmBedroomFigure *figure, const BfmBedroomView *view, unsigned texture_member,
    BfmWoodsEarthScrollForm form, const int16_t xyz[3], unsigned animation_updates,
    unsigned size_eighths)
{
    if (!shrine || !figure || !view || !xyz || texture_member != 59 || size_eighths > 8 ||
        (unsigned)form > BFM_EARTH_SCROLL_ECHO) return 0;
    int center[3];
    if (!size_eighths || !bfm_bedroom_project(view, xyz, center)) return 1;
    unsigned bucket = (unsigned)center[2] / 4 + 1;
    if (bucket >= 4096) return 1;
    const ScrollCell *cell = &shrine->detached_panel;
    if (form != BFM_EARTH_SCROLL_PANEL) {
        unsigned key = 0;
        while (key + 1 < SCROLL_KEYS && animation_updates >= shrine->scroll[key].end) ++key;
        cell = &shrine->scroll[key];
    }
    /* The camera-facing sprite projects its pivot and extent independently
     * at the child's depth. Rounding only the combined corner shifts a pixel. */
    int scale_y = form == BFM_EARTH_SCROLL_PANEL ? 7168 : 14336;
    BfmBedroomView plane = {.focal = view->focal, .translation = {0, 0, center[2]},
        .axes = {{11264 * (int)size_eighths / 8, 0, 0},
                 {0, scale_y * (int)size_eighths / 8, 0}, {0, 0, 4096}}};
    int pivot[3], extent[3];
    const int16_t offset[3] = {cell->left, cell->top, 0};
    const int16_t span[3] = {cell->width, cell->height, 0};
    if (!bfm_bedroom_project(&plane, offset, pivot) ||
        !bfm_bedroom_project(&plane, span, extent)) return 1;
    int left = center[0] + pivot[0] - 160, top = center[1] + pivot[1] - 120;
    int echo = form == BFM_EARTH_SCROLL_ECHO;
    BfmForestDustCard card = {.bucket = bucket,
        .material = {.page = (uint16_t)(cell->page | (echo ? 0x40 : 0)),
                     .clut = cell->clut, .count = 4, .textured = 1, .translucent = echo}};
    for (unsigned i = 0; i < 4; ++i) {
        card.vertices[i] = (BfmForestRasterVertex){
            .x = left + (i & 1 ? extent[0] - 160 : 0), .y = top + (i & 2 ? extent[1] - 120 : 0),
            .u = (uint8_t)(cell->u + (i & 1 ? cell->width : 0)),
            .v = (uint8_t)(cell->v + (i & 2 ? cell->height : 0)), .rgb = 0x808080};
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, &card, 1);
}

int bfm_woods_earth_shrine_view_glow(BfmBedroomFigure *figure, const BfmBedroomView *view,
    const int16_t xyz[3], int tablet_attached, int scale_x, int scale_y, uint32_t rgb)
{
    if (!figure || !view || !xyz || scale_x <= 0 || scale_y <= 0 ||
        scale_x > 24576 || scale_y > 24576) return 0;
    int32_t center[3];
    for (unsigned i = 0; i < 3; ++i) {
        int64_t sum = 0;
        for (unsigned j = 0; j < 3; ++j) {
            int coordinate = xyz[j] - (tablet_attached && j == 1 ? 160 : 0);
            sum += (int64_t)view->axes[i][j] * coordinate;
        }
        center[i] = view->translation[i] + fixed_to_integer(sum);
    }
    /* Move the light toward the camera on the same line of sight. Its
     * projected radius and depth change without sliding off the tablet. */
    int depth = center[2] - 384;
    if (depth <= 0) return 1;
    for (unsigned i = 0; i < 2; ++i)
        center[i] = (int32_t)((int64_t)center[i] * depth / center[2]);
    center[2] = depth;
    /* The effect stores a whole world coordinate before the field renderer
     * projects it again. Keep the camera-to-world quantization at that step. */
    int32_t world[3];
    for (unsigned i = 0; i < 3; ++i) {
        int64_t sum = 0;
        for (unsigned j = 0; j < 3; ++j)
            sum += (int64_t)view->axes[j][i] * (center[j] - view->translation[j]);
        world[i] = fixed_to_integer(sum);
        if (world[i] < INT16_MIN || world[i] > INT16_MAX) return 1;
    }
    BfmForestCamera camera;
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[6];
    size_t count = bfm_woods_radial_light_project_axes(&camera, world, 128,
                                                      scale_x, scale_y, rgb, cards, 6);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}

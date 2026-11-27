/* SPDX-License-Identifier: MIT */
#include "woods_earth_scroll_touch.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

static const uint8_t *shape(const uint8_t *scene, size_t bytes, uint32_t address)
{
    size_t offset = address - UINT32_C(0x80128158);
    if (offset > bytes || bytes - offset < 16 || u32le(scene + offset)) return NULL;
    return scene + offset;
}

static int body_shape(const uint8_t *scene, size_t bytes, uint32_t address, BfmForestGuardBox *body)
{
    const uint8_t *record = shape(scene, bytes, address);
    if (!record) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        body->low[axis] = s16le(record + 4 + 4 * axis);
        body->high[axis] = s16le(record + 6 + 4 * axis);
        if (body->low[axis] > body->high[axis]) return 0;
    }
    return 1;
}

int bfm_woods_earth_scroll_touch_mount(const uint8_t *scene, size_t bytes,
                                      BfmWoodsEarthScrollTouch *out)
{
    const size_t room_entry = UINT32_C(0x80190ce4) - UINT32_C(0x80128158);
    if (!scene || !out || bytes < room_entry + 4 ||
        u32le(scene + room_entry) != UINT32_C(0x801961e8)) return 0;
    const uint8_t *emblem = shape(scene, bytes, UINT32_C(0x801959a8));
    BfmWoodsEarthScrollTouch loaded = {0};
    if (!emblem ||
        !body_shape(scene, bytes, UINT32_C(0x801926e0), &loaded.hero_body[BFM_EARTH_TOUCH_ORDINARY]) ||
        !body_shape(scene, bytes, UINT32_C(0x80193590), &loaded.hero_body[BFM_EARTH_TOUCH_ATTACHED]))
        return 0;
    loaded.edge_x[0] = s16le(emblem + 4);
    loaded.edge_x[1] = s16le(emblem + 6);
    loaded.edge_y = s16le(emblem + 10);
    loaded.edge_z[0] = s16le(emblem + 12);
    loaded.edge_z[1] = s16le(emblem + 14);
    if (loaded.edge_x[0] > loaded.edge_x[1] || loaded.edge_z[0] > loaded.edge_z[1]) return 0;
    loaded.mounted = 1;
    *out = loaded;
    return 1;
}

int bfm_woods_earth_scroll_touch_test(const BfmWoodsEarthScrollTouch *touch,
                                     const BfmForestTransform *emblem_model,
                                     const BfmForestTransform *hero_model,
                                     BfmEarthTouchHeroBody body_kind)
{
    if (!touch || !touch->mounted || !emblem_model || !hero_model ||
        (body_kind != BFM_EARTH_TOUCH_ORDINARY && body_kind != BFM_EARTH_TOUCH_ATTACHED)) return 0;
    BfmForestTransform body = {.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}};
    memcpy(body.translation, hero_model->translation, sizeof body.translation);
    const BfmForestTransform *basis = body_kind == BFM_EARTH_TOUCH_ORDINARY ? &body : hero_model;
    for (unsigned i = 0; i < 2; ++i) {
        int16_t local[3] = {touch->edge_x[0], touch->edge_y, touch->edge_z[i]};
        int16_t start[3], end[3], center[3];
        bfm_forest_lift_world(emblem_model, local, start);
        local[0] = touch->edge_x[1];
        bfm_forest_lift_world(emblem_model, local, end);
        if (bfm_forest_guard_contact(&touch->hero_body[body_kind], basis, start, end, center)) return 1;
    }
    return 0;
}

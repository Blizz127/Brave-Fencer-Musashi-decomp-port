/* SPDX-License-Identifier: MIT */
#include "inn_keeper.h"
#include "bedroom_visitor.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { KEEPER_PARTS = 10, MAX_IDLE_POSES = 32 };

struct BfmInnKeeper {
    BfmBedroomVisitor *body;
    int position[3], yaw;
    unsigned count;
    uint32_t poses[MAX_IDLE_POSES];
};

static const uint8_t *scene_at(const uint8_t *scene, size_t bytes,
                               uint32_t address, size_t length)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (!scene || address < origin || length > bytes || address - origin > bytes - length)
        return NULL;
    return scene + (address - origin);
}

/* Each key in the mounted keeper idle holds a packed pose for one update. */
static int read_idle(BfmInnKeeper *keeper, const uint8_t *scene, size_t bytes, uint32_t idle)
{
    for (unsigned i = 0; i <= MAX_IDLE_POSES; ++i) {
        const uint8_t *key = scene_at(scene, bytes, idle + 8 * i, 8);
        if (!key) return 0;
        uint32_t tagged = u32le(key), duration = u32le(key + 4);
        if (!tagged && duration == 0xffff) {
            keeper->count = i;
            return i != 0;
        }
        uint32_t pose = tagged & ~UINT32_C(0x01000000);
        if (i == MAX_IDLE_POSES || (tagged >> 24) != 0x81 || duration != 1 ||
            (pose & 3) || !scene_at(scene, bytes, pose, KEEPER_PARTS * 8)) return 0;
        keeper->poses[i] = pose;
    }
    return 0;
}

static BfmInnKeeper *load_keeper(const uint8_t *scene, size_t bytes, uint32_t model,
                                 uint32_t idle, uint32_t placement, uint32_t shadow)
{
    const uint8_t *place = scene_at(scene, bytes, placement, 8);
    const uint8_t *table = scene_at(scene, bytes, model, (KEEPER_PARTS + 1) * 4);
    if (!place || !table || !u32le(table + 4 * (KEEPER_PARTS - 1)) ||
        u32le(table + 4 * KEEPER_PARTS)) return NULL;
    BfmInnKeeper *keeper = calloc(1, sizeof(*keeper));
    if (!keeper) return NULL;
    if (!read_idle(keeper, scene, bytes, idle)) {
        free(keeper);
        return NULL;
    }
    for (unsigned i = 0; i < 3; ++i) keeper->position[i] = s16le(place + 2 * i);
    keeper->yaw = u16le(place + 6) & 4095u;
    keeper->body = bfm_bedroom_visitor_load(scene, bytes, &model, 1, shadow);
    if (!keeper->body) {
        free(keeper);
        return NULL;
    }
    return keeper;
}

BfmInnKeeper *bfm_inn_keeper_load(const uint8_t *scene, size_t bytes)
{
    return load_keeper(scene, bytes, 0x801f968c, 0x801c3790, 0x801f3534, 0x80190fb0);
}

BfmInnKeeper *bfm_vambee_inn_keeper_load(const uint8_t *scene, size_t bytes)
{
    /* The outbreak inn supplies the placement. Its resident street member
     * now holds both the keeper's model and his idle animation. */
    return load_keeper(scene, bytes, 0x801cea24, 0x801beaf8, 0x801e669c, 0x8018e654);
}

void bfm_inn_keeper_free(BfmInnKeeper *keeper)
{
    if (!keeper) return;
    bfm_bedroom_visitor_free(keeper->body);
    free(keeper);
}

int bfm_inn_keeper_add(const BfmInnKeeper *keeper, BfmBedroomFigure *figure,
                       const BfmBedroomView *view, unsigned age)
{
    return keeper && bfm_bedroom_visitor_add(keeper->body, figure, view, 0,
                        keeper->position, keeper->yaw, keeper->poses[age % keeper->count]);
}

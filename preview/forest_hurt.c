/* SPDX-License-Identifier: MIT */
#include "forest_hurt.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

int bfm_forest_hurt_load(const MusashiDiscMedia *disc, BfmForestHurtClip *out)
{
    /* The grounded recipient selects between these two directional motions. */
    static const uint32_t motions[] = {UINT32_C(0x800d58ac), UINT32_C(0x800d5880)};
    ForestPlayerArchive archive;
    BfmForestHurtClip clip = {0};
    uint32_t parts = 0;
    if (!out || !forest_player_archive_open(disc, &archive)) return 0;
    for (unsigned side = 0; side < 2; ++side) {
        uint8_t script[4 * (3 + BFM_FOREST_HURT_KEYS + 1)];
        if (!forest_player_archive_read(&archive, motions[side], script, sizeof script) ||
            u32le(script) != 4 || u32le(script + 8) != 19 || u32le(script + sizeof script - 4))
            return 0;
        uint32_t list = u32le(script + 4);
        if ((list & 3) || (side && list != parts)) return 0;
        parts = list;
        for (unsigned i = 0; i < BFM_FOREST_HURT_KEYS; ++i) {
            uint32_t key = u32le(script + 12 + 4 * i);
            uint8_t packed[256];
            if ((key & 3) || !forest_player_archive_read(&archive, key, packed, sizeof packed))
                return 0;
            clip.pose[side][i] = key + 16;
        }
    }
    uint8_t list[4 * (MUSASHI_FOREST_PLAYER_PARTS + 1)];
    if (!forest_player_archive_read(&archive, parts, list, sizeof list) || u32le(list + sizeof list - 4))
        return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if ((u32le(list + 4 * i) >> 24) != 0x80) return 0;
        clip.visible |= UINT32_C(1) << i;
    }
    /* The creek player carries Fusion. Hide the spare blade and Lumina pieces. */
    clip.visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 24) | (UINT32_C(1) << 25));
    *out = clip;
    return 1;
}

int bfm_forest_hurt_begin(BfmForestHurt *hurt, int body_yaw, int hit_yaw)
{
    if (!hurt || hurt->active) return 0;
    unsigned delta = ((unsigned)body_yaw - (unsigned)hit_yaw) & 4095u;
    unsigned separation = delta < 2048 ? delta : 4095 - delta;
    *hurt = (BfmForestHurt){.active = 1, .side = separation > 1024};
    return 1;
}

BfmForestHurtResult bfm_forest_hurt_step(BfmForestHurt *hurt, const BfmForestHurtClip *clip,
    ForestPlayerState *actor, BfmForestManual *manual, uint32_t *pose)
{
    if (!hurt || !clip || !actor || !manual || !pose) return BFM_FOREST_HURT_INVALID;
    if (!hurt->active) return BFM_FOREST_HURT_FREE;
    unsigned key = hurt->next_key;
    if (hurt->side > 1 || key > BFM_FOREST_HURT_KEYS || !clip->visible)
        return BFM_FOREST_HURT_INVALID;
    if (key == BFM_FOREST_HURT_KEYS) {
        hurt->active = 0;
        actor->kind = actor->state = 0;
        actor->action = actor->phase = 1;
        return BFM_FOREST_HURT_RECOVERED;
    }
    uint32_t frame = clip->pose[hurt->side][key];
    if (!frame) return BFM_FOREST_HURT_INVALID;
    if (!key) {
        manual->moving = manual->after_jump = manual->left_support = 0;
        manual->motion = manual->elapsed = 0;
        manual->last_descent = 0;
        actor->field_24.word = actor->field_2c.word = actor->local_y.word = 0;
        memset(actor->velocity, 0, sizeof actor->velocity);
        actor->field_1a8 = 0;
        actor->kind = actor->state = 25;
        actor->action = 2;
        actor->phase = 1;
    }
    *pose = frame;
    ++hurt->next_key;
    return BFM_FOREST_HURT_LOCKED;
}

BfmForestBodySample bfm_forest_hurt_display(BfmForestHurtDisplay *display,
                                             BfmForestBodySample current)
{
    BfmForestBodySample prior = display->history[0];
    display->history[0] = display->history[1];
    display->history[1] = current;
    return prior.pose ? prior : current;
}

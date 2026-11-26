/* SPDX-License-Identifier: MIT */
#include "steam_knight_street_route.h"
#include <limits.h>

static const uint8_t *row(const uint8_t *scene, size_t bytes, uint32_t address, size_t size)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || address - base > bytes || size > bytes - (address - base))
        return NULL;
    return scene + (address - base);
}

static uint16_t half(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}

int bfm_steam_street_route_load(BfmSteamKnightStreetRoute *out,
                                const uint8_t *scene, size_t bytes)
{
    const uint8_t *slot = row(scene, bytes, UINT32_C(0x801959e8), 4);
    const uint8_t *depth = row(scene, bytes, UINT32_C(0x80195acc), 2);
    const uint8_t *markers = row(scene, bytes, UINT32_C(0x80195af4), 8 * BFM_STREET_MARKERS);
    if (!out || !slot || !depth || !markers) return 0;
    uint32_t table = half(slot) | (uint32_t)half(slot + 2) << 16;
    if ((table & 3u) || table > UINT32_MAX - 4u * BFM_STREET_ROWS) return 0;
    BfmSteamKnightStreetRoute route = {0};
    route.depth_limit = (int16_t)half(depth);
    for (unsigned i = 0; i < BFM_STREET_MARKERS; ++i)
        route.marker_tags[i] = (int16_t)half(markers + 8*i + 4);
    for (unsigned i = 0; i <= BFM_STREET_ROWS; ++i) {
        const uint8_t *entry = row(scene, bytes, table + 4*i, 4);
        if (!entry) return 0;
        int16_t delay = (int16_t)half(entry);
        if (delay == -1) {
            if (!i) return 0;
            route.length = i;
            route.delay = route.rows[0].delay;
            *out = route;
            return 1;
        }
        uint16_t action = half(entry + 2);
        unsigned code = action & 0xffu;
        if (i == BFM_STREET_ROWS || delay < 0 || (action & ~0x1ffu) ||
            (code != 1 && code != 5 && code != 6 && code != 7 && code != 8 && code != 9)) return 0;
        route.rows[i].delay = delay;
        route.rows[i].action = action;
    }
    return 0;
}

int bfm_steam_street_route_choose(BfmSteamKnightStreetRoute *r, uint32_t flags,
                                  int hp, int marker, BfmSteamStreetChoice *out)
{
    if (!r || !out || !r->length || r->length > BFM_STREET_ROWS || r->cursor >= r->length)
        return 0;
    BfmSteamStreetChoice choice = {.action = BFM_STREET_IDLE, .marker = -1};
    if (!(flags & 8u) && r->pending_walks) {
        --r->pending_walks;
        choice.action = flags & 0x800u ? BFM_STREET_LEFT : BFM_STREET_FORWARD;
    } else if (!r->delay && !(flags & 0x200u)) {
        uint16_t raw = r->rows[r->cursor].action;
        unsigned code = raw & 0xffu;
        int matches = marker >= 0 && marker < BFM_STREET_MARKERS &&
                      (unsigned)r->marker_tags[marker] == code;
        int can_queue = (flags & 10u) == 2u;
        switch (code) {
        case 1:
            choice.action = BFM_STREET_SHOT;
            break;
        case 5:
            if (!(flags & 0x408u))
                choice.action = flags & 0x800u ? BFM_STREET_LEFT : BFM_STREET_FORWARD;
            break;
        case 6:
        case 7:
            if (hp > 128 || (flags & 0x800u))
                choice.action = code == 6 ? BFM_STREET_JUMP : BFM_STREET_CHOICE_SEVEN;
            break;
        case 8:
            if (flags & 0x800u) break;
            if (!(flags & 0x400u)) {
                if (can_queue) ++r->pending_walks;
                *out = choice;
                return 1;
            }
            choice.action = matches ? BFM_STREET_MARKED_JUMP : BFM_STREET_JUMP;
            if (matches) choice.marker = marker;
            break;
        case 9:
            choice.action = matches ? BFM_STREET_MARKED_SHOT : BFM_STREET_SHOT;
            if (matches) choice.marker = marker;
            break;
        default:
            return 0;
        }
        if ((raw & 0x100u) && can_queue) ++r->pending_walks;
        r->cursor = (r->cursor + 1u) % r->length;
        r->delay = r->rows[r->cursor].delay;
    }
    *out = choice;
    return 1;
}

void bfm_steam_street_delay_tick(BfmSteamKnightStreetRoute *r)
{
    if (r && r->delay > 0) --r->delay;
}

void bfm_steam_street_depth_crossed(BfmSteamKnightStreetRoute *r, uint32_t *flags)
{
    if (r && flags) {
        *flags |= 0x400u;
        r->pending_walks = 0;
    }
}

int bfm_steam_street_walk_begin(BfmSteamKnightStreetWalk *walk, const int32_t root_q12[3],
                                uint32_t body_flags)
{
    if (!walk || !root_q12 || (body_flags & 0x800u)) return 0;
    BfmSteamKnightStreetWalk value = {
        .root_q12 = {root_q12[0], root_q12[1], root_q12[2]},
        .ticks = body_flags & 0x2000u ? 24u : 48u,
        .step_q12 = body_flags & 0x2000u ? 7 * 2048 : 7 * 1024
    };
    *walk = value;
    return 1;
}

BfmSteamKnightStreetStep bfm_steam_street_walk_tick(BfmSteamKnightStreetWalk *walk,
                                                    int16_t limit_z)
{
    if (!walk || !walk->ticks || walk->age >= walk->ticks || walk->step_q12 <= 0 ||
        walk->root_q12[2] > INT32_MAX - walk->step_q12) return BFM_STREET_WALK_INVALID;
    walk->root_q12[2] += walk->step_q12;
    ++walk->age;
    /* Bounds compare the signed integer coordinate, including any overshoot. */
    if ((int64_t)walk->root_q12[2] >= ((int32_t)limit_z + 1) * INT64_C(4096)) {
        walk->ticks = walk->age;
        return BFM_STREET_WALK_CROSSED_DEPTH;
    }
    return walk->age == walk->ticks ? BFM_STREET_WALK_EXPIRED : BFM_STREET_WALK_MOVING;
}

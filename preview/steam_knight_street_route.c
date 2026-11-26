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
    const uint8_t *limits = row(scene, bytes, UINT32_C(0x80195acc), 4);
    const uint8_t *markers = row(scene, bytes, UINT32_C(0x80195af4), 8 * (BFM_STREET_MARKERS + 1));
    if (!out || !slot || !limits || !markers || (int16_t)half(markers + 8 * BFM_STREET_MARKERS + 4) != -1)
        return 0;
    uint32_t table = half(slot) | (uint32_t)half(slot + 2) << 16;
    if ((table & 3u) || table > UINT32_MAX - 4u * BFM_STREET_ROWS) return 0;
    BfmSteamKnightStreetRoute route = {0};
    route.depth_limit = (int16_t)half(limits);
    route.west_limit = (int16_t)half(limits + 2);
    for (unsigned i = 0; i < BFM_STREET_MARKERS; ++i) {
        const uint8_t *mark = markers + 8*i;
        route.marker_tags[i] = (int16_t)half(mark + 4);
        route.markers[i] = (BfmSteamStreetMarker){
            .x = (int16_t)half(mark), .z = (int16_t)half(mark + 2),
            .phase = (int16_t)half(mark + 6)
        };
    }
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

static int lateral(int delta)
{
    return delta < -42 ? -84 : delta > 42 ? 84 : 0;
}

int bfm_steam_street_roll_phase(const BfmSteamKnightStreetRoute *route, int marker,
                               uint32_t flags, int16_t x)
{
    if (!route || marker < 0 || marker >= BFM_STREET_MARKERS ||
        (flags & 0xc08u) != 0x800u || route->marker_tags[marker] != 5) return 0;
    const BfmSteamStreetMarker *row = &route->markers[marker];
    return x < row->x ? row->phase : 0;
}

int bfm_steam_street_jump_target(const BfmSteamKnightStreetRoute *route, int marker,
    unsigned jumps_left, uint32_t flags, const int16_t action_start[3],
    const int16_t body[3], const int16_t hero[3], int16_t xz[2])
{
    if (!route || !action_start || !body || !hero || !xz || !jumps_left ||
        marker < -1 || marker >= BFM_STREET_MARKERS) return 0;
    if (marker >= 0) {
        xz[0] = route->markers[marker].x;
        xz[1] = route->markers[marker].z;
    } else if (jumps_left == 1) {
        xz[0] = action_start[0];
        xz[1] = action_start[2];
    } else if (flags & 0x800u) {
        int cells = ((int16_t)(hero[0] - body[0]) + 42) / 84;
        if (cells < 0) cells = 0;
        if (cells > 3) cells = 3;
        xz[0] = (int16_t)(body[0] + 84 * cells);
        xz[1] = (int16_t)(2912 + lateral((int16_t)(hero[2] - 2912)));
    } else {
        int cells = ((int16_t)(hero[2] - body[2]) + 42) / 84;
        if (cells < -3) cells = -3;
        if (cells > 0) cells = 0;
        xz[0] = (int16_t)(-128 + lateral((int16_t)(hero[0] + 128)));
        xz[1] = (int16_t)(body[2] + 84 * cells);
    }
    return 1;
}

int bfm_steam_street_walk_begin(BfmSteamKnightStreetWalk *walk, const int32_t root_q12[3],
                                uint32_t body_flags)
{
    if (!walk || !root_q12) return 0;
    BfmSteamKnightStreetWalk value = {
        .root_q12 = {root_q12[0], root_q12[1], root_q12[2]},
        .west = !!(body_flags & 0x800u),
        .ticks = body_flags & 0x2000u ? 24u : 48u,
        .step_q12 = body_flags & 0x2000u ? 7 * 2048 : 7 * 1024
    };
    *walk = value;
    return 1;
}

BfmSteamKnightStreetStep bfm_steam_street_walk_tick(BfmSteamKnightStreetWalk *walk,
                                                    int16_t boundary)
{
    if (!walk || !walk->ticks || walk->age >= walk->ticks || walk->step_q12 <= 0)
        return BFM_STREET_WALK_INVALID;
    unsigned axis = walk->west ? 0 : 2;
    int64_t next = (int64_t)walk->root_q12[axis] + (walk->west ? -walk->step_q12 : walk->step_q12);
    if (next < INT32_MIN || next > INT32_MAX) return BFM_STREET_WALK_INVALID;
    walk->root_q12[axis] = (int32_t)next;
    ++walk->age;
    /* Bounds compare the signed integer coordinate, including any overshoot. */
    if (walk->west && next < (int64_t)boundary * 4096) {
        walk->ticks = walk->age;
        return BFM_STREET_WALK_CROSSED_WEST;
    }
    if (!walk->west && next >= ((int32_t)boundary + 1) * INT64_C(4096)) {
        walk->ticks = walk->age;
        return BFM_STREET_WALK_CROSSED_DEPTH;
    }
    return walk->age == walk->ticks ? BFM_STREET_WALK_EXPIRED : BFM_STREET_WALK_MOVING;
}

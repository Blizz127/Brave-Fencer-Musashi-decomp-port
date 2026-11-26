/* SPDX-License-Identifier: MIT */
#include "steam_knight_flail.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <math.h>

enum { WORLD_ONE = 65536, FLOOR_Y = -2048 };

int bfm_steam_flail_contacts_load(BfmSteamFlailContacts *out, const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    const uint32_t shape[] = {UINT32_C(0x80192290), UINT32_C(0x801963fc)};
    if (!out || !scene) return 0;
    BfmSteamFlailContacts value;
    BfmForestGuardBox *boxes[] = {&value.hero, &value.head};
    for (unsigned i = 0; i < 2; ++i) {
        size_t offset = shape[i] - base;
        if (offset > bytes || bytes - offset < 16 || u32le(scene + offset)) return 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            boxes[i]->low[axis] = s16le(scene + offset + 4 + 4 * axis);
            boxes[i]->high[axis] = s16le(scene + offset + 6 + 4 * axis);
            if (boxes[i]->low[axis] >= boxes[i]->high[axis]) return 0;
        }
    }
    *out = value;
    return 1;
}

int bfm_steam_flail_player_hit(const BfmSteamFlailContacts *contacts, int marker,
                              const int16_t position[3], const BfmForestTransform *hero,
                              int16_t contact[3], int16_t normal[3])
{
    if (!contacts || marker >= 0 || !position || !hero || !contact || !normal) return 0;
    const BfmForestGuardBox *head = &contacts->head;
    for (int tier = 0; tier < 2; ++tier) {
        int y = position[1] + head->high[1] - 8 - 32 * tier;
        for (int lane = 0; lane < 4; ++lane) {
            int z = position[2] + head->low[0] + 32 * lane;
            int16_t from[3] = {(int16_t)(position[0] + head->low[0]), (int16_t)y, (int16_t)z};
            int16_t to[3] = {(int16_t)(position[0] + head->high[0]), (int16_t)y, (int16_t)z};
            int16_t center[3];
            if (bfm_forest_guard_contact_point(&contacts->hero, hero, from, to, center,
                                               contact, normal)) return 1;
        }
    }
    return 0;
}

int bfm_steam_flail_wind(const BfmForestTransform *socket, unsigned *reach,
                         unsigned *angle, BfmForestTransform *head)
{
    if (!socket || !reach || !angle || !head || *reach > 161 || *angle >= 4096) return 0;
    unsigned radius = *reach < 160 ? *reach + 5 : *reach;
    unsigned yaw = (*angle + 4096 - 256) % 4096;
    BfmForestTransform spin, moved = *socket;
    const int16_t origin[3] = {0};
    bfm_forest_actor_root(yaw, origin, &spin);
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t v = (int64_t)socket->translation[axis] +
                    ((spin.rotation[axis][2] * (int)radius) >> 12);
        if (v < INT16_MIN || v > INT16_MAX) return 0;
        moved.translation[axis] = (int32_t)v;
    }
    *reach = radius;
    *angle = yaw;
    *head = moved;
    return 1;
}

int bfm_steam_flail_reel(const BfmForestTransform *socket, const int16_t endpoint[3],
                         unsigned remaining, BfmForestTransform *head)
{
    if (!socket || !endpoint || !head || remaining > 3) return 0;
    BfmForestTransform moved = *socket;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int32_t origin = socket->translation[axis];
        if (origin < INT16_MIN || origin > INT16_MAX) return 0;
        moved.translation[axis] = origin + ((endpoint[axis] - origin) * (int)remaining >> 2);
    }
    *head = moved;
    return 1;
}

int bfm_steam_flail_launch(BfmSteamKnightFlail *flail, const int16_t head[3],
    int16_t body_y, const int16_t player[3], const BfmSteamKnightStreetRoute *route,
    int marker, uint32_t flags)
{
    if (!flail || !head || !player || !route || marker < -1 ||
        marker >= BFM_STREET_MARKERS) return 0;
    const BfmSteamStreetMarker *mark = marker >= 0 ? &route->markers[marker] : NULL;
    int target[3] = {mark ? mark->x : player[0], body_y, mark ? mark->z : player[2]};
    int delta[3];
    double squared = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        delta[axis] = target[axis] - head[axis];
        if (delta[axis] < INT16_MIN || delta[axis] > INT16_MAX) return 0;
        squared += (double)delta[axis] * delta[axis];
    }
    BfmSteamKnightFlail next = {.marker = marker, .active = 1};
    double length = sqrt(squared);
    int speed = flags & 0x2000u ? 1152 : 1024;
    for (unsigned axis = 0; axis < 3; ++axis) {
        next.position[axis] = (int32_t)head[axis] * WORLD_ONE;
        int unit = length > 0 ? (int)((double)delta[axis] * 4096.0 / length) : 0;
        next.velocity[axis] = unit * speed;
    }
    *flail = next;
    return 1;
}

BfmSteamFlailStep bfm_steam_flail_step(BfmSteamKnightFlail *flail,
                                       const BfmSteamKnightStreetRoute *route)
{
    if (!flail || !flail->active || !route) return BFM_STEAM_FLAIL_INVALID;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t moved = (int64_t)flail->position[axis] + flail->velocity[axis];
        if (moved < INT32_MIN || moved > INT32_MAX) return BFM_STEAM_FLAIL_INVALID;
    }
    for (unsigned axis = 0; axis < 3; ++axis)
        flail->position[axis] += flail->velocity[axis];
    int ground = (flail->position[1] >> 16) >= FLOOR_Y;
    if (ground) {
        flail->position[1] = FLOOR_Y * WORLD_ONE + (uint16_t)flail->position[1];
        flail->velocity[1] = 0;
    }
    int marker = flail->marker;
    if (marker >= 0 && marker < BFM_STREET_MARKERS &&
        (flail->position[2] >> 16) <= route->markers[marker].z) {
        flail->position[2] = (int32_t)route->markers[marker].z * WORLD_ONE +
                             (uint16_t)flail->position[2];
        flail->active = 0;
        return BFM_STEAM_FLAIL_AT_MARKER;
    }
    if (ground) {
        flail->active = 0;
        return BFM_STEAM_FLAIL_GROUNDED;
    }
    return BFM_STEAM_FLAIL_MOVING;
}

/* SPDX-License-Identifier: MIT */
#include "nightmare_stride.h"

#include <stdlib.h>

struct BfmNightmareStride {
    const uint8_t *scene;
    size_t bytes;
    uint32_t room;
    BfmBedroomTravel *walk;
    int height_eighths, speed_eighths;
    int flying, landed, arrived;
    int16_t waypoint_height;
};

BfmNightmareStride *bfm_nightmare_stride_open(const uint8_t *scene, size_t bytes,
                                               uint32_t room_address)
{
    if (!scene || !bytes) return NULL;
    BfmNightmareStride *s = calloc(1, sizeof *s);
    if (s) {
        s->scene = scene;
        s->bytes = bytes;
        s->room = room_address;
    }
    return s;
}

void bfm_nightmare_stride_free(BfmNightmareStride *s)
{
    if (!s) return;
    bfm_bedroom_travel_free(s->walk);
    free(s);
}

int bfm_nightmare_stride_hop(BfmNightmareStride *s, BfmBedroomRoot *root)
{
    if (!s || !root || s->walk) return -1;
    if (s->landed) return 1;

    BfmBedroomRoot next = *root;
    /* For the vertical hop on this level bed, the shadow intersects the
     * same face as the falling feet. Its draw position is two units above
     * the geometry, which is also the clearance of the grounded player. */
    if (!bfm_bedroom_travel_air_shadow(s->scene, s->bytes, s->room, &next) ||
        !next.shadow_visible || next.shadow_normal[0] || next.shadow_normal[2] ||
        next.shadow_normal[1] >= -3584) return -1;
    if (!s->flying) {
        if (next.xyz[1] != next.shadow_xyz[1]) return -1;
        s->height_eighths = 8 * next.xyz[1];
        s->speed_eighths = -18 * 8;
        s->flying = 1;
    }

    int speed = s->speed_eighths + 13;
    int height = s->height_eighths + speed;
    int whole = height / 8 - (height % 8 < 0);
    if (whole > INT16_MAX || whole < INT16_MIN) return -1;
    if (speed > 0 && whole > next.shadow_xyz[1] + 2) {
        s->landed = 1;
        next.xyz[1] = next.shadow_xyz[1];
        for (unsigned axis = 0; axis < 3; ++axis)
            next.normal[axis] = next.shadow_normal[axis];
    } else {
        next.xyz[1] = (int16_t)whole;
    }
    s->speed_eighths = speed;
    s->height_eighths = height;
    *root = next;
    return s->landed;
}

int bfm_nightmare_stride_walk(BfmNightmareStride *s, BfmBedroomRoot *root,
                              const int16_t waypoint[3],
                              const int32_t eye[3], const int32_t target[3])
{
    if (!s || !root || !waypoint || !eye || !target || !s->landed) return -1;
    if (s->arrived) return 1;

    if (!s->walk) {
        s->walk = bfm_bedroom_travel_from_root(s->scene, s->bytes, s->room, root);
        if (!s->walk || !bfm_bedroom_travel_walk(s->walk, waypoint[0], waypoint[2], 0))
            return -1;
        s->waypoint_height = waypoint[1];
        /* This room reads the waypoint and applies its first stick input in
         * one update. Consume the general travel queue's dispatch now. */
        if (!bfm_bedroom_travel_step(s->walk, eye, target)) return -1;
    }
    /* The first reach test measures all three coordinates. The second
     * horizontal test replaces the waypoint height for the next update. */
    int16_t previous_height = s->waypoint_height;
    int64_t dx = waypoint[0] - root->xyz[0], dy = previous_height - root->xyz[1];
    int64_t dz = waypoint[2] - root->xyz[2];
    s->waypoint_height = root->xyz[1];
    if (!bfm_bedroom_travel_step(s->walk, eye, target)) return -1;
    *root = *bfm_bedroom_travel_root(s->walk);
    if (root->busy || dx*dx + dy*dy + dz*dz >= 16*16) return 0;
    root->xyz[1] = previous_height;
    if (!bfm_bedroom_travel_air_shadow(s->scene, s->bytes, s->room, root)) return -1;
    s->arrived = 1;
    return 1;
}

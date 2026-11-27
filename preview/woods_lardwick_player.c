/* SPDX-License-Identifier: MIT */
#include "woods_lardwick_player.h"
#include "field_waypoint.h"
#include "forest_lift.h"
#include "woods_twinpeak_stride.h"

#include <string.h>

int bfm_woods_lardwick_player_queue(BfmWoodsLardwickPlayer *player, unsigned command,
    uint32_t argument, const BfmForestTransform *soldier_model, const int16_t soldier_xyz[3])
{
    if (!player || player->count >= 2) return 0;
    int16_t world[3];
    if (command == 4) {
        int16_t local[3] = {(int16_t)argument, 0, (int16_t)(argument >> 16)};
        if (!soldier_model) return 0;
        bfm_forest_lift_world(soldier_model, local, world);
    } else if (command == 10 && soldier_xyz) {
        memcpy(world, soldier_xyz, sizeof world);
    } else return 0;
    player->orders[player->count].command = command;
    player->orders[player->count].x = world[0];
    player->orders[player->count].y = world[1];
    player->orders[player->count].z = world[2];
    ++player->count;
    player->ready = 0;
    return 1;
}

int bfm_woods_lardwick_player_step(BfmWoodsLardwickPlayer *player, BfmVillageMotion *motion,
                                   int camera_yaw, int player_ready)
{
    if (!player || !motion) return 0;
    player->ready = player_ready && !player->count;
    if (!player_ready || !player->count) return 1;

    BfmBedroomRoot root = bfm_village_motion_player(motion)->root;
    int16_t previous_height = root.xyz[1];
    int32_t position[3];
    if (!bfm_village_motion_position_q16(motion, position)) return 0;
    int dx = player->orders[0].x - root.xyz[0], dz = player->orders[0].z - root.xyz[2];
    root.locomotion_edge = BFM_TRAVEL_NONE;
    int walking = 0, done;
    if (player->orders[0].command == 4) {
        const BfmVillageGround *ground = bfm_village_motion_ground(motion);
        int dy = player->orders[0].y - root.xyz[1];
        int64_t horizontal = (int64_t)dx * dx + (int64_t)dz * dz;
        /* The arrival test includes the previous target height. A second
         * horizontal test settles the feet and records their height for
         * the following visit, even if the first test is still pending. */
        done = horizontal + (int64_t)dy * dy < 16 * 16;
        if (horizontal < 16 * 16) {
            int16_t to[3] = {player->orders[0].x, root.xyz[1], player->orders[0].z};
            if (!bfm_twinpeak_footing(ground, &root, to)) return 0;
        } else {
            BfmWaypointInput stick = bfm_waypoint_input(dx, dz, 0, camera_yaw);
            root.yaw = (unsigned)stick.yaw;
            if (!bfm_twinpeak_stride(ground, &root, position, stick.speed_q16, root.yaw)) return 0;
            walking = 1;
        }
        if (walking != (int)player->walking)
            root.locomotion_edge = walking ? BFM_TRAVEL_WALK : BFM_TRAVEL_STAND;
    } else {
        done = bfm_twinpeak_turn(&root, (unsigned)bfm_waypoint_heading(-dx, -dz));
    }
    if (!bfm_village_motion_staged(motion, &root)) return 0;
    for (unsigned i = 0; i < 3; ++i)
        position[i] = (int32_t)root.xyz[i] * 65536 + (uint16_t)position[i];
    if (!bfm_village_motion_staged_position(motion, position)) return 0;
    player->orders[0].y = previous_height;
    player->walking = walking;
    if (done) {
        --player->count;
        if (player->count) player->orders[0] = player->orders[1];
    }
    return 1;
}

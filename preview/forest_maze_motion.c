/* SPDX-License-Identifier: MIT */
#include "forest_maze_motion.h"
#include "forest_terrain.h"
#include "gameplay_run.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

struct BfmForestMazeMotion {
    const MusashiDiscMedia *disc;
    BfmForestBankHop hop;
    BfmForestBankHopPose *poses;
    BfmForestBankHopRing ring;
    BfmBedroomRoot launch_root;
    int launch, rebound;
};

BfmForestMazeMotion *bfm_forest_maze_motion_open(const MusashiDiscMedia *disc,
                                                  const uint8_t *scene, size_t bytes)
{
    /* SC03/7 has the shared ability catalog at its own linked address. */
    size_t price = UINT32_C(0x8018a0b8) - UINT32_C(0x80128158) + 12;
    if (!disc || !scene || bytes <= price || !(scene[price] & 0x80)) return NULL;
    BfmForestMazeMotion *maze = calloc(1,sizeof *maze);
    if (maze) {
        maze->disc = disc;
        maze->hop.cost = scene[price] & 0x7f;
    }
    return maze;
}

void bfm_forest_maze_motion_free(BfmForestMazeMotion *maze)
{
    if (maze) bfm_forest_bank_hop_pose_free(maze->poses);
    free(maze);
}

static int load_hop(BfmForestMazeMotion *maze)
{
    if (!maze->poses) maze->poses = bfm_forest_bank_hop_pose_open(maze->disc);
    return maze->poses && (maze->ring.dust.atlas.width ||
                           bfm_forest_bank_hop_ring_load(&maze->ring,maze->disc));
}

int bfm_forest_maze_motion_step(BfmForestMazeMotion *maze, BfmVillageMotion *motion,
    BfmGameplayRun *run, BfmFieldInput field, int camera_yaw, unsigned *sound)
{
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    if (!maze || !p || !sound) return 0;
    *sound = 0;
    maze->launch = maze->rebound = 0;
    int equipped = run && run->player.record[0x49] == 12;
    int controlled = p->controlled;
    if (controlled && equipped) {
        if (!load_hop(maze)) return 0;
        int was_active = maze->hop.active;
        if (!was_active) maze->launch_root = p->root;
        int consumed = bfm_village_motion_bank_hop(motion,&maze->hop,maze->poses,&run->player,
                                                    field,camera_yaw,0,&maze->launch);
        if (consumed < 0) return 0;
        if (consumed) {
            maze->rebound = was_active && maze->launch;
            if (maze->rebound) maze->launch_root = p->root;
            if (maze->launch) *sound = 0x47a;
            return 1;
        }
    } else if (controlled && run) {
        /* The field clock counts a remaining ability duration even after
         * cancellation or arrival from another room with its old timer. */
        if (maze->hop.active) bfm_forest_bank_hop_pose_reset(maze->poses);
        maze->hop.active = 0;
        uint8_t *timer = run->player.record + 0x50;
        uint32_t remaining = u32le(timer);
        if (remaining) {
            --remaining;
            for (unsigned i = 0; i < 4; ++i) timer[i] = (uint8_t)(remaining >> (8*i));
        }
    }
    if (!bfm_village_motion_step_maze_field(motion,field.x,field.y,camera_yaw,
                                            field.cross_pressed,field.cross_held,run)) return 0;
    if (!controlled && p->controlled && equipped) {
        if (!load_hop(maze) || !bfm_village_motion_maze_resume_hop(motion,&maze->hop,maze->poses))
            return 0;
        maze->launch_root = p->root;
        maze->launch = 1;
        *sound = 0x47a;
    }
    return 1;
}

int16_t bfm_forest_maze_tracking_y(const BfmForestMazeMotion *maze, int16_t foot_y)
{
    return maze ? bfm_forest_bank_hop_camera_y(&maze->hop,foot_y) : foot_y;
}

void bfm_forest_maze_motion_effects(BfmForestMazeMotion *maze, const BfmVillageCamera *camera)
{
    if (!maze || !camera) return;
    bfm_forest_bank_hop_ring_step(&maze->ring,&maze->launch_root,camera->eye,maze->rebound,maze->launch);
    maze->launch = maze->rebound = 0;
}

unsigned bfm_forest_maze_motion_cards(const BfmForestMazeMotion *maze,
    const BfmVillageCamera *camera, BfmForestDustCard *cards, unsigned capacity)
{
    if (!maze || !camera || !maze->ring.dust.atlas.width || camera->focal <= 0 ||
        camera->focal > UINT16_MAX) return 0;
    int near_target[3];
    for (unsigned i = 0; i < 3; ++i)
        near_target[i] = camera->eye[i] + (camera->target[i] - camera->eye[i])/2;
    BfmForestCamera view;
    if (!bfm_forest_camera_look_at(camera->eye,near_target,(uint16_t)camera->focal,&view)) return 0;
    return bfm_woods_foot_dust_project(&maze->ring.dust,&maze->ring.pool,&view,cards,capacity);
}

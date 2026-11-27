/* SPDX-License-Identifier: MIT */
#include "woods_jump_pose.h"
#include "woods_fusion_art.h"

int bfm_woods_jump_pose_load(BfmWoodsJumpPose *out, const MusashiDiscMedia *disc,
                             uint32_t table)
{
    BfmWoodsJumpPose poses;
    if (!out || !bfm_forest_jump_poses_load(disc, table, &poses.standing) ||
        !bfm_forest_walkoff_poses_load(disc, table, &poses.stride)) return 0;
    *out = poses;
    return 1;
}

int bfm_woods_jump_pose_apply(const BfmWoodsJumpPose *jump, int stride,
                              enum BfmForestJumpPhase phase, unsigned phase_tick,
                              const uint8_t *main, size_t main_bytes, BfmBedroomPose *body)
{
    if (!jump) return 0;
    const BfmForestJumpPoses *clip = stride ? &jump->stride : &jump->standing;
    uint32_t frame = bfm_forest_jump_pose_frame(clip, phase, phase_tick);
    if (!frame || !bfm_woods_fusion_art_pose(main, main_bytes, frame, clip->visible, 0, body))
        return 0;
    body->show_lumina = 0;
    return 1;
}

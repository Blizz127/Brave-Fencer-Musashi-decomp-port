/* SPDX-License-Identifier: MIT */
#include "tower_summit_camera.h"
#include "forest_pose.h"
#include "forest_shore_motion.h"

int bfm_tower_summit_camera(const int16_t player[3], BfmForestCamera *camera,
                            int32_t eye[3])
{
    if (!player || !camera || !eye)
        return 0;

    /* The summit looks a little to the player's right and above the head.
     * Its arm makes one eighth of a turn from the player's radial bearing. */
    const int32_t target[3] = {player[0] + 80, player[1] - 200, player[2]};
    unsigned yaw = bfm_forest_shore_bearing(player[0], player[2]) + 512;
    BfmForestTransform arm;
    bfm_forest_actor_pitch_yaw(128, yaw, player, &arm);
    int32_t short_target[3];
    for (unsigned i = 0; i < 3; ++i) {
        int product = 1000 * arm.rotation[i][2];
        int distance = product / 4096 - (product % 4096 < 0);
        eye[i] = target[i] + distance;
        /* Half the ray fits the shared field-camera normalization range. */
        short_target[i] = eye[i] - distance / 2;
    }
    return bfm_forest_camera_look_at(eye, short_target, 300, camera);
}

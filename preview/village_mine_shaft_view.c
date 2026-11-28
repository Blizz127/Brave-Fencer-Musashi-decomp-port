/* SPDX-License-Identifier: MIT */
#include "village_mine_shaft_view.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"

BfmVillageMineView *bfm_village_mine_shaft_view_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024])
{
    const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x80181f28);
    const size_t binding = UINT32_C(0x8017e300) - base;
    if (!scene || bytes < room - base + 0xcc || u32le(scene + binding) != room) return NULL;
    return bfm_village_mine_view_open_field(disc, scene, bytes, room, UINT32_C(0x80180178), 109, tiles);
}

void bfm_village_mine_shaft_view_follow(BfmVillageMineView *view, const int16_t foot[3])
{
    if (!view || !foot) return;
    BfmVillageCamera camera = {.focal = 400, .radius = 1200, .yaw = 2048, .following = 1};
    /* The upper ledges let the camera travel farther sideways than the lower floor. */
    int min_x = foot[1] < -1664 ? -736 : -576;
    int max_x = foot[1] < -1664 ? 704 : 576;
    int16_t aim[3] = {foot[0], (int16_t)(foot[1] - 32), foot[2]};
    if (aim[0] < min_x) aim[0] = (int16_t)min_x;
    if (aim[0] > max_x) aim[0] = (int16_t)max_x;
    BfmForestTransform orbit;
    int16_t eye[3], arm[3] = {0, 0, (int16_t)camera.radius};
    bfm_forest_actor_pitch_yaw(341, (unsigned)camera.yaw, aim, &orbit);
    bfm_forest_lift_world(&orbit, arm, eye);
    for (unsigned i = 0; i < 3; ++i) {
        camera.target[i] = aim[i];
        camera.eye[i] = eye[i];
    }
    bfm_village_mine_view_set_camera(view, &camera);
}

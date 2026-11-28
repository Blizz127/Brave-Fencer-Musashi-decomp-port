/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_view.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

enum { STRIP_PHASES = 16, STRIP_WIDTH = 8, STRIP_HEIGHT = 64, TRANSFER_BYTES = 12 };

int bfm_village_mine_lake_view_open(BfmVillageMineLakeView *lake, const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024])
{
    const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x80185a50);
    const size_t binding = UINT32_C(0x80181d20) - base, zoom = UINT32_C(0x80185984) - base;
    const size_t animation = UINT32_C(0x8019bb90) - base;
    const size_t table_bytes = STRIP_PHASES * TRANSFER_BYTES;
    if (!lake || !scene || bytes < animation + 2 * table_bytes || u32le(scene + binding) != room)
        return 0;
    BfmVillageMineLakeView next = {.tiles = tiles, .phase = -1};
    for (unsigned strip = 0; strip < 2; ++strip) {
        next.transfers[strip] = scene + animation + strip * table_bytes;
        for (unsigned i = 0; i < STRIP_PHASES; ++i) {
            const uint8_t *row = next.transfers[strip] + i * TRANSFER_BYTES;
            if (u16le(row + 4) != STRIP_WIDTH || u16le(row + 6) != STRIP_HEIGHT ||
                u16le(row) > 1024 - STRIP_WIDTH || u16le(row + 2) > 512 - STRIP_HEIGHT ||
                u16le(row + 8) > 1024 - STRIP_WIDTH || u16le(row + 10) > 512 - STRIP_HEIGHT)
                return 0;
        }
    }
    for (unsigned i = 0; i < 2; ++i) {
        next.zoom_radius[i] = s16le(scene + zoom + 2 * i);
        if (next.zoom_radius[i] <= 0) return 0;
    }
    next.radius = next.zoom_radius[0];
    next.view = bfm_village_mine_view_open_field(disc, scene, bytes, room,
                                                 UINT32_C(0x80183b98), 110, tiles);
    if (!next.view) return 0;
    *lake = next;
    return 1;
}

void bfm_village_mine_lake_view_close(BfmVillageMineLakeView *lake)
{
    if (lake) {
        bfm_village_mine_view_free(lake->view);
        *lake = (BfmVillageMineLakeView){0};
    }
}

void bfm_village_mine_lake_view_follow(BfmVillageMineLakeView *lake, const int16_t foot[3],
                                      unsigned pressed)
{
    if (!lake || !lake->view || !foot) return;
    if (pressed & 3u) lake->zoom ^= 1u;
    int delta = lake->zoom_radius[lake->zoom] - lake->radius;
    int step = delta / 4;
    if (!step && delta) step = delta > 0 ? 1 : -1;
    lake->radius += step;

    int16_t aim[3] = {foot[0], foot[1], foot[2]};
    if (aim[0] < -2880) aim[0] = -2880;
    if (aim[0] > 2368) aim[0] = 2368;
    if (aim[1] > -768) aim[1] = -768;
    aim[1] -= 80;

    BfmForestTransform orbit;
    int16_t eye[3], arm[3] = {0, 0, (int16_t)lake->radius};
    bfm_forest_actor_pitch_yaw(227, 2048, aim, &orbit);
    bfm_forest_lift_world(&orbit, arm, eye);
    BfmVillageCamera camera = {.focal = 400, .radius = lake->radius, .yaw = 2048, .following = 1};
    for (unsigned axis = 0; axis < 3; ++axis) {
        camera.target[axis] = aim[axis];
        camera.eye[axis] = eye[axis];
    }
    bfm_village_mine_view_set_camera(lake->view, &camera);
}

void bfm_village_mine_lake_view_tick_tiles(BfmVillageMineLakeView *lake)
{
    if (!lake || !lake->view) return;
    if (lake->phase < 0) {
        lake->phase = 0;
        return;
    }
    for (unsigned strip = 0; strip < 2; ++strip) {
        const uint8_t *row = lake->transfers[strip] + lake->phase * TRANSFER_BYTES;
        unsigned x = u16le(row), y = u16le(row + 2);
        unsigned dest_x = u16le(row + 8), dest_y = u16le(row + 10);
        for (unsigned line = 0; line < STRIP_HEIGHT; ++line)
            memmove(&lake->tiles[dest_y + line][dest_x], &lake->tiles[y + line][x],
                    STRIP_WIDTH * sizeof(uint16_t));
    }
    lake->phase = (lake->phase + 1) % STRIP_PHASES;
}

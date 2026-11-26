/* SPDX-License-Identifier: MIT */
#include "tower_interior.h"
#include "tower_interior_fixtures.h"
#include "tower_interior_bell.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>

enum { ROOM = 0x300a, FOCAL = 500, KEY_BYTES = 14, CIRCLE = 4096 };
static const double radians = 6.28318530717958647693 / CIRCLE;

struct BfmTowerInterior {
    BfmForestField *room;
    const uint8_t *track;
    unsigned updates;
    unsigned age;
    BfmTowerInteriorBell *bell;
    BfmTowerInteriorFixtures fixtures;
};

BfmTowerInterior *bfm_tower_interior_open(const MusashiDiscMedia *disc,
                                          const RoomTransferDescriptor *transfer)
{
    if (!disc || !transfer || transfer->room != ROOM || transfer->world || transfer->parameter)
        return NULL;
    BfmTowerInterior *interior = calloc(1, sizeof *interior);
    if (!interior) return NULL;
    interior->room = bfm_forest_field_open(disc, ROOM);
    const uint8_t *door = bfm_forest_field_entrance(interior->room, 0);
    size_t bytes = 0;
    const uint8_t *scene = bfm_forest_field_scene(interior->room, &bytes);
    /* The US entrance camera installs an interleaved eye/aim/roll table. */
    const size_t offset = UINT32_C(0x801b67c8) - UINT32_C(0x80128158);
    if (door && scene && offset <= bytes && u32le(door + 24) == UINT32_C(0x8017d960)) {
        interior->track = scene + offset;
        size_t count = (bytes - offset) / KEY_BYTES;
        for (size_t i = 0; i < count; ++i) {
            if (s16le(interior->track + i * KEY_BYTES + 12) == INT16_MAX) {
                if (i && bfm_tower_interior_fixtures_load(scene, bytes, &interior->fixtures)) {
                    interior->bell = bfm_tower_interior_bell_open(scene, bytes);
                    if (!interior->bell) break;
                    interior->updates = (unsigned)i;
                    return interior;
                }
                break;
            }
        }
    }
    bfm_tower_interior_free(interior);
    return NULL;
}

void bfm_tower_interior_free(BfmTowerInterior *interior)
{
    if (interior) {
        bfm_tower_interior_bell_free(interior->bell);
        bfm_forest_field_free(interior->room);
        free(interior);
    }
}

BfmForestField *bfm_tower_interior_room(BfmTowerInterior *interior)
{
    return interior ? interior->room : NULL;
}

int bfm_tower_interior_place(BfmTowerInterior *interior, ForestPlayerState *player,
                             int16_t *yaw, int16_t position[3])
{
    return interior && bfm_forest_place_player(interior->room, 0, player, yaw, position);
}

unsigned bfm_tower_interior_intro_updates(const BfmTowerInterior *interior)
{
    return interior ? interior->updates : 0;
}

static int camera_view(const int32_t eye[3], const int32_t target[3], int roll,
                       BfmForestCamera *camera)
{
    /* Keep the lookup-based field normalization in its supported range. Scale
     * the viewing ray without changing the absolute camera position. */
    int32_t delta[3], origin[3] = {0};
    for (unsigned i = 0; i < 3; ++i) delta[i] = target[i] - eye[i];
    while (abs(delta[0]) > 724 || abs(delta[1]) > 724 || abs(delta[2]) > 724 ||
           delta[0] * delta[0] + delta[2] * delta[2] >= 524288)
        for (unsigned i = 0; i < 3; ++i) delta[i] /= 2;
    if (!bfm_forest_camera_look_at(origin, delta, FOCAL, camera)) return 0;
    if (roll) {
        /* Each key supplies roll in 1/128 degree units. The eye's roll
         * rotates the world oppositely in view space. */
        double angle = -roll * (6.28318530717958647693 / (360.0 * 128.0));
        double c = cos(angle), s = sin(angle);
        for (unsigned i = 0; i < 3; ++i) {
            int x = camera->rotation[0][i], y = camera->rotation[1][i];
            camera->rotation[0][i] = (int16_t)lround(c*x - s*y);
            camera->rotation[1][i] = (int16_t)lround(s*x + c*y);
        }
    }
    for (unsigned i = 0; i < 3; ++i) {
        int64_t dot = 0;
        for (unsigned j = 0; j < 3; ++j) dot -= (int64_t)eye[j] * camera->rotation[i][j];
        camera->translation[i] = (int32_t)(dot / CIRCLE - (dot % CIRCLE < 0));
    }
    return 1;
}

int bfm_tower_interior_camera(const BfmTowerInterior *interior, const int16_t player[3],
                              unsigned shot_update, int follow_player,
                              BfmForestCamera *camera, int32_t eye[3])
{
    if (!interior || !player || !camera || !eye) return 0;
    int32_t target[3];
    int roll = 0;
    if (!follow_player) {
        if (shot_update >= interior->updates) shot_update = interior->updates - 1;
        const uint8_t *key = interior->track + shot_update * KEY_BYTES;
        for (unsigned i = 0; i < 3; ++i) {
            eye[i] = s16le(key + i * 2);
            target[i] = s16le(key + 6 + i * 2);
        }
        roll = s16le(key + 12);
    } else {
        double bearing = bfm_forest_shore_bearing(player[0], player[2]) * radians;
        double pitch = 96 * radians;
        target[0] = player[0] + (int32_t)(sin(bearing - 1024 * radians) * 128);
        target[1] = player[1] - 128;
        target[2] = player[2] + (int32_t)(cos(bearing - 1024 * radians) * 128);
        eye[0] = target[0] + (int32_t)floor(800 * sin(bearing) * cos(pitch));
        eye[1] = target[1] - (int32_t)ceil(800 * sin(pitch));
        eye[2] = target[2] + (int32_t)floor(800 * cos(bearing) * cos(pitch));
    }
    return camera_view(eye, target, roll, camera);
}

void bfm_tower_interior_tick(BfmTowerInterior *interior, BfmGameplayRun *run)
{
    if (!interior || !run) return;
    if (interior->fixtures.started)
        ++interior->age;
    else
        bfm_tower_interior_fixtures_start(&interior->fixtures, run);
}

int bfm_tower_interior_draw(const BfmTowerInterior *interior, const BfmForestCamera *camera,
                            const BfmForestPose *pose, const BfmForestTransform *root,
                            uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                            uint32_t pixels[240][320])
{
    if (!interior || !pixels) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x)
            pixels[y][x] = UINT32_C(0xff000000);
    BfmForestDustCard cards[BFM_TOWER_FIXTURE_CARDS];
    size_t count = bfm_tower_interior_fixture_cards(&interior->fixtures, interior->age,
                                                     camera, cards, sizeof cards / sizeof cards[0]);
    BfmForestActor bell;
    if (!bfm_tower_interior_bell_actor(interior->bell, interior->age, &bell)) return 0;
    return bfm_forest_draw_room(interior->room, camera, pose, root, visible_parts, buffer,
                                blade, &bell, 1, cards, count, pixels);
}

/* SPDX-License-Identifier: MIT */
#include "tower_interior.h"
#include "tower_summit_departure.h"
#include "tower_interior_fixtures.h"
#include "tower_bell_impact.h"
#include "tower_bell_effect.h"
#include "forest_lesson_fusion.h"
#include "forest_muzzle_art.h"
#include "forest_shot_effects.h"
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
    int ascent_gain, previous_gain;
    BfmTowerInteriorBell *bell;
    BfmTowerBellSound *sound;
    BfmTowerBellImpact impact;
    BfmTowerBellEffect light;
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
                    if (!interior->bell || !bfm_tower_bell_impact_load(scene, bytes, &interior->impact) ||
                        !bfm_tower_bell_effect_load(scene, bytes, &interior->light)) break;
                    interior->ascent_gain = interior->previous_gain = -1;
                    interior->sound = bfm_tower_bell_sound_load(disc);
                    if (!interior->sound) break;
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
        bfm_tower_bell_sound_free(interior->sound);
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
    BfmTowerBellResponse response = {0};
    int reacting = bfm_tower_interior_response(interior, &response);
    if (reacting && response.camera != BFM_TOWER_BELL_PLAYER_CAMERA) {
        static const int32_t side[] = {896, -512, 0};
        static const int32_t overhead[] = {320, -2176, -320};
        const int32_t *anchor = response.camera == BFM_TOWER_BELL_SIDE_CAMERA ? side : overhead;
        for (unsigned i = 0; i < 3; ++i) eye[i] = anchor[i];
        target[0] = 0;
        target[1] = -1088;
        target[2] = 0;
    } else if (!follow_player) {
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
    if (reacting && response.shake) {
        eye[1] += response.shake;
        target[1] += response.shake;
    }
    return camera_view(eye, target, roll, camera);
}

/* The wind follows progress around the helix. Height resolves overlapping turns. */
static int ascent_gain(const int16_t player[3])
{
    int y = player[1];
    unsigned angle = (0u - bfm_forest_shore_bearing(-player[2], player[0])) & 4095;
    int progress = 0;
    if (y < -1537) {
        progress = 12288;
    } else if (y < -96 && (angle < 3072 || y < -128)) {
        int nearest = 32767;
        for (unsigned turn = 0; turn < 3; ++turn) {
            int candidate = (int)(angle + turn * 4096);
            int difference = abs(y + 34 + candidate / 8);
            if (difference <= nearest) {
                nearest = difference;
                progress = candidate;
            }
        }
    }
    return progress >= 7808 ? 127 : 127 - (7808 - progress) * 127 / 7808;
}

void bfm_tower_interior_tick(BfmTowerInterior *interior, BfmGameplayRun *run)
{
    if (!interior || !run) return;
    interior->previous_gain = interior->ascent_gain;
    bfm_tower_interior_bell_tick(interior->bell);
    BfmTowerBellResponse response;
    if (bfm_tower_interior_response(interior, &response) && response.age == 117)
        bfm_tower_bell_effect_start(&interior->light, run);
    if (interior->fixtures.started)
        ++interior->age;
    else
        bfm_tower_interior_fixtures_start(&interior->fixtures, run);
}

void bfm_tower_interior_grounded_sound(BfmTowerInterior *interior, const int16_t feet[3])
{
    if (interior && feet && interior->age > 20) interior->ascent_gain = ascent_gain(feet);
}

int bfm_tower_interior_gunshot_contact(void *context, const int16_t from[3],
                                      const int16_t to[3], int16_t hit[3], int16_t normal[3])
{
    BfmTowerInterior *interior = context;
    int16_t clipped[3];
    unsigned surface = bfm_forest_resolve_solid_segment(interior->room, from, to, clipped);
    const int16_t *end = surface & 0xe000 ? clipped : to;
    if (bfm_tower_interior_bell_contact(interior->bell, interior->age, 16, from, end, hit, normal))
        return 1;
    if (end == to) return 0;
    for (unsigned i = 0; i < 3; ++i) hit[i] = clipped[i];
    return -1;
}

int bfm_tower_interior_response(const BfmTowerInterior *interior, BfmTowerBellResponse *response)
{
    return interior && bfm_tower_interior_bell_response(interior->bell, response);
}

const RoomTransferDescriptor *bfm_tower_interior_exit(const BfmTowerInterior *interior,
                                                      const int16_t feet[3])
{
    BfmForestContact support;
    if (!interior || !feet || !bfm_tower_interior_bell_destination(interior->bell) ||
        !bfm_forest_ground(interior->room, feet[0], feet[1], feet[2], &support))
        return NULL;
    return bfm_tower_interior_bell_door_contact(interior->bell, &support, feet);
}

BfmTowerBellSound *bfm_tower_interior_sound(const BfmTowerInterior *interior,
                                            unsigned *command, unsigned *argument)
{
    BfmTowerBellResponse reaction;
    if (!interior || !command || !argument) return NULL;
    if (bfm_tower_interior_response(interior, &reaction)) {
        if (!reaction.sound_command) return NULL;
        *command = reaction.sound_command;
        *argument = reaction.sound_argument;
    } else {
        if (interior->previous_gain == interior->ascent_gain || interior->ascent_gain < 0) return NULL;
        *command = 1213;
        *argument = 0x1000u | (unsigned)interior->ascent_gain;
    }
    return interior->sound;
}

int bfm_tower_interior_draw(const BfmTowerInterior *interior, const BfmForestCamera *camera,
                            const BfmForestPose *pose, const BfmForestTransform *root,
                            uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                            uint32_t pixels[240][320])
{
    return bfm_tower_interior_draw_lift(interior, camera, pose, root, visible_parts,
                                        buffer, blade, NULL, pixels);
}

int bfm_tower_interior_draw_lift(const BfmTowerInterior *interior, const BfmForestCamera *camera,
                            const BfmForestPose *pose, const BfmForestTransform *root,
                            uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                            const BfmTowerSummitFrame *lift, uint32_t pixels[240][320])
{
    if (!interior || !pixels) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x)
            pixels[y][x] = UINT32_C(0xff000000);
    BfmForestDustCard cards[1 + BFM_TOWER_BELL_RAY_CARDS + BFM_TOWER_FIXTURE_CARDS + BFM_FOREST_MUZZLE_CARDS +
                            BFM_GUNSHOT_FAN_CARDS + BFM_FOREST_SHOT_EFFECT_CARDS + BFM_TOWER_SUMMIT_DEPARTURE_CARDS];
    size_t count = bfm_tower_interior_fixture_cards(&interior->fixtures, interior->age,
                                                     camera, cards, BFM_TOWER_FIXTURE_CARDS);
    count += bfm_forest_field_project_gunshots(interior->room, camera, cards + count,
                                                sizeof cards / sizeof cards[0] - count);
    BfmTowerBellResponse reaction = {0};
    int responding = bfm_tower_interior_response(interior, &reaction);
    if (bfm_tower_bell_impact_card(&interior->impact, interior->age, reaction.room_phase,
                                   reaction.phase_age, camera, &cards[count])) ++count;
    if (responding && reaction.age >= 117)
        count += bfm_tower_bell_effect_cards(&interior->light, reaction.age - 117,
            camera, &cards[count], sizeof cards / sizeof *cards - count);
    BfmForestTransform body;
    if (lift && lift->wide_camera) {
        size_t bytes;
        const uint8_t *scene = bfm_forest_field_scene(interior->room, &bytes);
        const int16_t pad[3] = {(int16_t)lift->goal_x, (int16_t)lift->goal_y, (int16_t)lift->goal_z};
        count += bfm_tower_summit_departure_cards(scene, bytes, pad, lift->beam_age, buffer,
            camera, cards + count, sizeof cards / sizeof *cards - count);
        body = *root;
        bfm_tower_summit_departure_hero(lift->beam_age, &body);
        if (!lift->hero_visible) {
            visible_parts = 0;
            blade = NULL;
        }
        root = &body;
    }
    BfmForestActor bell;
    if (!bfm_tower_interior_bell_actor(interior->bell, interior->age, &bell)) return 0;
    if (!bfm_forest_draw_room(interior->room, camera, pose, root, visible_parts, buffer,
                              blade, &bell, 1, cards, count, pixels)) return 0;
    if (responding && reaction.white) {
        unsigned add = reaction.white >> 3;
        for (unsigned y = 0; y < 240; ++y) for (unsigned x = 0; x < 320; ++x) {
            uint32_t rgb = pixels[y][x], output = UINT32_C(0xff000000);
            for (unsigned shift = 0; shift <= 16; shift += 8) {
                unsigned level = ((rgb >> shift & 255) >> 3) + add;
                output |= ((level > 31 ? 31 : level) * 255 / 31) << shift;
            }
            pixels[y][x] = output;
        }
    }
    return 1;
}

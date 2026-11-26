/* SPDX-License-Identifier: MIT */
#include "tower_interior_bell.h"
#include "forest_guard_contact.h"
#include "forest_entry.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

static const uint32_t scene_origin = UINT32_C(0x80128158);

struct BfmTowerInteriorBell {
    MusashiForestPlayerMesh *mesh;
    BfmForestPose pose;
    BfmForestTransform root;
    int16_t position[3];
    int16_t door[3];
    BfmForestGuardBox box, door_box;
    struct { int8_t delta; uint8_t ticks; } shake[12];
    RoomTransferDescriptor destination;
    uint16_t yaw;
    int struck;
    unsigned age;
    unsigned speed, acceleration;
};

static const uint8_t *mounted(const uint8_t *scene, size_t size, uint32_t address, size_t len)
{
    if (!scene || address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= size && len <= size - offset ? scene + offset : NULL;
}

BfmTowerInteriorBell *bfm_tower_interior_bell_open(const uint8_t *scene, size_t size)
{
    const uint8_t *records = mounted(scene, size, UINT32_C(0x801a3060) + 0xb4, 4);
    if (!records) return NULL;
    const uint8_t *spawn = mounted(scene, size, u32le(records), 20);
    const uint8_t *swing = mounted(scene, size, UINT32_C(0x8018a9d0), 4);
    if (!spawn || u16le(spawn + 6) != 22 || !swing ||
        s16le(swing) <= 0 || s16le(swing + 2) <= 0 || s16le(swing) % s16le(swing + 2))
        return NULL;
    BfmTowerInteriorBell *bell = calloc(1, sizeof *bell);
    if (!bell) return NULL;
    unsigned parts = 0;
    bell->mesh = musashi_forest_player_mesh_from_table(scene, size, scene_origin,
                                                       UINT32_C(0x801be010), &parts);
    if (!bell->mesh || parts != 1) {
        bfm_tower_interior_bell_free(bell);
        return NULL;
    }
    const uint8_t *shape = mounted(scene, size, UINT32_C(0x8018a9c0), 16);
    const uint8_t *door = mounted(scene, size, UINT32_C(0x8018a9d8), 20);
    const uint8_t *shake = mounted(scene, size, UINT32_C(0x8018a9a8), 24);
    const uint8_t *door_box = mounted(scene, size, UINT32_C(0x80186d7c), 16);
    if (!door_box || u32le(door_box) || !shake || !shape || u32le(shape) != 0 || !door || u16le(door + 6) != 124)
        goto invalid;
    for (unsigned i = 0; i < 12; ++i) {
        bell->shake[i].delta = (int8_t)shake[2*i];
        bell->shake[i].ticks = shake[2*i+1];
    }
    const uint8_t *transfer = mounted(scene, size, u32le(door + 16), 36);
    if (!transfer) goto invalid;
    bfm_forest_decode_transfer(transfer, &bell->destination);
    if (bell->destination.room != 0x300b) goto invalid;
    for (unsigned i = 0; i < 3; ++i) {
        bell->position[i] = s16le(spawn + 2*i);
        bell->door[i] = s16le(door + 2*i);
        bell->box.low[i] = s16le(shape + 4 + 4*i);
        bell->box.high[i] = s16le(shape + 6 + 4*i);
        bell->door_box.low[i] = s16le(door_box + 4 + 4*i);
        bell->door_box.high[i] = s16le(door_box + 6 + 4*i);
        if (bell->box.low[i] > bell->box.high[i] ||
            bell->door_box.low[i] > bell->door_box.high[i]) goto invalid;
    }
    bell->yaw = u16le(spawn + 12);
    bell->speed = u16le(swing);
    bell->acceleration = u16le(swing + 2);
    /* The idle clip binds the resident zero-translation, zero-rotation pose. */
    bfm_forest_actor_root(0, (const int16_t[3]){0}, &bell->pose.local[0]);
    return bell;
invalid:
    bfm_tower_interior_bell_free(bell);
    return NULL;
}

void bfm_tower_interior_bell_free(BfmTowerInteriorBell *bell)
{
    if (bell) {
        musashi_forest_player_mesh_free(bell->mesh);
        free(bell);
    }
}

enum {
    DROP_PERIOD = 39, DROP_UPDATES = 25, DROPS = 3,
    SHAFT_START = DROPS * DROP_PERIOD, RESTORE_LIGHT = SHAFT_START + 256,
    OPEN_DOOR = RESTORE_LIGHT + 32
};

int bfm_tower_interior_bell_contact(BfmTowerInteriorBell *bell, unsigned room_age,
    unsigned attack_class, const int16_t from[3], const int16_t to[3],
    int16_t hit[3], int16_t normal[3])
{
    if (!bell || bell->struck || room_age < 20 || attack_class != 16 ||
        !from || !to || !hit || !normal) return 0;
    /* The box shares the model translation. The tilt belongs to the drawing. */
    BfmForestTransform root;
    int16_t center[3];
    bfm_forest_actor_root(0, bell->position, &root);
    if (!bfm_forest_guard_contact_point(&bell->box, &root, from, to, center, hit, normal))
        return 0;
    bell->struck = 1;
    return 1;
}

void bfm_tower_interior_bell_tick(BfmTowerInteriorBell *bell)
{
    /* One last update settles the model after the gate appears. */
    if (bell && bell->struck && bell->age <= OPEN_DOOR) ++bell->age;
}

int bfm_tower_interior_bell_response(const BfmTowerInteriorBell *bell,
                                     BfmTowerBellResponse *response)
{
    if (!bell || !bell->struck || !response) return 0;
    unsigned age = bell->age;
    BfmTowerBellResponse state = {.age=age, .locked=age < OPEN_DOOR,
                                  .door_open=age >= OPEN_DOOR};
    if (age < SHAFT_START) {
        unsigned cycle = age / DROP_PERIOD;
        unsigned drop_age = age % DROP_PERIOD;
        state.room_phase = 2 * cycle + 1 + (drop_age >= DROP_UPDATES);
        state.phase_age = drop_age >= DROP_UPDATES ? drop_age - DROP_UPDATES : drop_age;
    } else {
        state.room_phase = 6;
        state.phase_age = age - (2*DROP_PERIOD + DROP_UPDATES);
    }
    if (!age) {
        state.sound_command = 1213;
        state.sound_argument = 0x1000;
    } else if (age < SHAFT_START && age % DROP_PERIOD == DROP_UPDATES) {
        state.sound_command = 1203;
    } else if (age == 2*DROP_PERIOD + 14) {
        state.sound_command = 1202;
    } else if (age == SHAFT_START) {
        state.sound_command = 4;
        state.sound_argument = 1213;
    }
    if (age >= 2*DROP_PERIOD-1 && age < RESTORE_LIGHT-1)
        state.camera = BFM_TOWER_BELL_OVERHEAD_CAMERA;
    else if (age >= DROP_PERIOD-1 && age < 2*DROP_PERIOD-1)
        state.camera = BFM_TOWER_BELL_SIDE_CAMERA;
    if (age >= DROP_UPDATES) {
        unsigned last = age >= 2*DROP_PERIOD + DROP_UPDATES ? 2 :
                        age >= DROP_PERIOD + DROP_UPDATES ? 1 : 0;
        unsigned t = age - (last*DROP_PERIOD + DROP_UPDATES);
        for (unsigned i = 0; i < 12 && bell->shake[i].ticks; ++i) {
            unsigned duration = 1 + bell->shake[i].ticks;
            if (t < duration) {
                state.shake = bell->shake[i].delta;
                break;
            }
            t -= duration;
        }
    }
    /* Amount added to each channel by the full-screen translucent polygon. */
    if (age > RESTORE_LIGHT-31 && age <= RESTORE_LIGHT)
        state.white = (age - (RESTORE_LIGHT-31))*8 - 1;
    else if (age > RESTORE_LIGHT && age < OPEN_DOOR)
        state.white = (OPEN_DOOR - age)*8;
    for (unsigned i = 0; i < 3; ++i) state.door[i] = bell->door[i];
    *response = state;
    return 1;
}

const RoomTransferDescriptor *bfm_tower_interior_bell_destination(const BfmTowerInteriorBell *bell)
{
    return bell && bell->struck && bell->age >= OPEN_DOOR ? &bell->destination : NULL;
}

const RoomTransferDescriptor *bfm_tower_interior_bell_door_contact(
    const BfmTowerInteriorBell *bell, const BfmForestContact *support, const int16_t feet[3])
{
    const RoomTransferDescriptor *destination = bfm_tower_interior_bell_destination(bell);
    if (!destination || !support || support->flags != 3 || !feet) return NULL;
    /* The door receives a vertical probe spanning the player's current floor
     * contact from three units above to one below. */
    for (unsigned axis = 0; axis < 3; ++axis) {
        int relative = feet[axis] - bell->door[axis];
        int first = relative - (axis == 1 ? 3 : 0);
        int last = relative + (axis == 1);
        if (last < bell->door_box.low[axis] || first > bell->door_box.high[axis]) return NULL;
    }
    return destination;
}

static int16_t dropped_height(const BfmTowerInteriorBell *bell)
{
    if (bell->age > OPEN_DOOR) return -1213;
    unsigned t = bell->age < SHAFT_START ? bell->age % DROP_PERIOD : DROP_UPDATES;
    if (t > DROP_UPDATES) t = DROP_UPDATES;
    /* Gravity is 3/4 world unit per update squared. Velocity advances before
     * position. After 25 updates the bell holds at the bottom of each drop. */
    return (int16_t)(bell->position[1] + (int)(3*t*(t+1)/8));
}

int bfm_tower_interior_bell_actor(BfmTowerInteriorBell *bell, unsigned elapsed_updates,
                                  BfmForestActor *out)
{
    if (!bell || !out) return 0;
    /* Constant angular acceleration opposes the displacement. Zero belongs
     * to the rising half, making the two arcs two updates different in length. */
    unsigned pitch = 0;
    if (!bell->struck && elapsed_updates > 20) {
        unsigned half = 2 * (bell->speed / bell->acceleration + 1);
        unsigned t = (elapsed_updates - 20) % (2 * half);
        int64_t angle;
        if (t <= half + 1) {
            angle = (int64_t)t * (bell->speed + bell->acceleration) -
                    (int64_t)bell->acceleration * t * ((int)t - 1) / 2;
        } else {
            t -= half + 1;
            angle = -(int64_t)t * bell->speed +
                    (int64_t)bell->acceleration * t * (t - 1) / 2;
        }
        pitch = (unsigned)angle & 4095;
    }
    int16_t position[3] = {bell->position[0], bell->position[1], bell->position[2]};
    if (bell->struck) position[1] = dropped_height(bell);
    bfm_forest_actor_pitch_yaw(pitch, bell->yaw, position, &bell->root);
    *out = (BfmForestActor){.mesh=bell->mesh, .pose=&bell->pose,
                            .root=&bell->root, .visible_parts=1};
    return 1;
}

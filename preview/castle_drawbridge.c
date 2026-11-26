/* SPDX-License-Identifier: MIT */
#include "castle_drawbridge.h"
#include <stdlib.h>
#include <string.h>

enum {
    SELECT_CASTLE = 30, PLACE_ACTOR = 124, REVEAL_START = 128, FULL_LIGHT = 150,
    BEGIN_CROSSING = 164, RELEASE_GATE = 287,
    BRIDGE_APPROACH_Z = 182, BRIDGE_LIP_Z = 128, GATE_LANDING_Z = -320, GATE_Z = -432
};

struct BfmCastleDrawbridge {
    BfmCastleDrawbridgeView *view;
    RoomTransferDescriptor entrance, exit;
    int16_t feet[3], heading;
    unsigned age, motion_tick;
    int airborne, vertical_speed_eighths, height_eighths;
    enum BfmDrawbridgeMotion motion;
    int request_taken;
};

BfmCastleDrawbridge *bfm_castle_drawbridge_open(const MusashiDiscMedia *disc,
    const RoomTransferDescriptor *from_summit, const BfmGameplayRun *run)
{
    if (!disc || !from_summit || from_summit->room != 0x300d ||
        from_summit->parameter || from_summit->world ||
        !run || !run->has_player || !bfm_gameplay_story_flag(run, 128))
        return NULL;
    BfmCastleDrawbridge *b = calloc(1, sizeof *b);
    if (!b) return NULL;
    b->view = bfm_castle_drawbridge_view_open(disc);
    BfmForestField *field = b->view ? bfm_castle_drawbridge_view_room(b->view) : NULL;
    const uint8_t *in = field ? bfm_forest_field_entrance(field, 0) : NULL;
    const uint8_t *out = field ? bfm_forest_field_entrance(field, 1) : NULL;
    if (in && out) {
        memcpy(&b->entrance, in, sizeof b->entrance);
        memcpy(&b->exit, out, sizeof b->exit);
        if (b->entrance.room == 0x300d && !b->entrance.world && !b->entrance.parameter &&
            b->exit.room == 0x300e && !b->exit.world && !b->exit.parameter) {
            b->feet[0] = (int16_t)b->entrance.x;
            b->feet[1] = (int16_t)b->entrance.y;
            b->feet[2] = (int16_t)b->entrance.z;
            b->heading = (int16_t)b->entrance.facing;
            return b;
        }
    }
    bfm_castle_drawbridge_free(b);
    return NULL;
}

void bfm_castle_drawbridge_free(BfmCastleDrawbridge *b)
{
    if (!b) return;
    bfm_castle_drawbridge_view_free(b->view);
    free(b);
}

static void set_motion(BfmCastleDrawbridge *b, enum BfmDrawbridgeMotion next)
{
    if (b->motion != next) {
        b->motion = next;
        b->motion_tick = 0;
    } else {
        ++b->motion_tick;
    }
}

int bfm_castle_drawbridge_tick(BfmCastleDrawbridge *b)
{
    if (!b) return -1;
    if (b->age == RELEASE_GATE) return 0;
    ++b->age;
    if (b->age < BEGIN_CROSSING || b->feet[2] == GATE_Z) {
        set_motion(b, BFM_DRAWBRIDGE_STAND);
        return 1;
    }
    BfmForestField *field = bfm_castle_drawbridge_view_room(b->view);
    int16_t old_z = b->feet[2];
    int z = old_z - (old_z > BRIDGE_APPROACH_Z || old_z <= GATE_LANDING_Z ? 11 : 10);
    if (z < GATE_Z) z = GATE_Z;
    BfmForestContact floor;
    if (!bfm_forest_ground(field, b->feet[0], b->feet[1] - 64, (int16_t)z, &floor))
        return -1;
    b->feet[2] = (int16_t)z;
    /* The scripted hop onto the wood is a short, released jump. Use the
     * normal running jump's vertical impulse and released-button gravity.
     * The floor probe already includes the actor's surface clearance. */
    if (old_z > BRIDGE_LIP_Z && z <= BRIDGE_LIP_Z) {
        b->airborne = 1;
        b->vertical_speed_eighths = -18 * 8;
        b->height_eighths = b->feet[1] * 8;
    }
    if (b->airborne) {
        b->vertical_speed_eighths += b->vertical_speed_eighths < 0 ? 21 : 13;
        b->height_eighths += b->vertical_speed_eighths;
        if (b->vertical_speed_eighths >= 0 && b->height_eighths >= floor.height * 8)
            b->airborne = 0;
    }
    b->feet[1] = b->airborne
        ? (int16_t)(b->height_eighths / 8 - (b->height_eighths % 8 < 0))
        : floor.height;
    set_motion(b, b->airborne ? BFM_DRAWBRIDGE_JUMP :
        (z > GATE_Z ? BFM_DRAWBRIDGE_RUN : BFM_DRAWBRIDGE_STAND));
    return 1;
}

unsigned bfm_castle_drawbridge_room(const BfmCastleDrawbridge *b)
{
    return b && b->age >= SELECT_CASTLE ? 0x300d : 0x300b;
}

int bfm_castle_drawbridge_descending(const BfmCastleDrawbridge *b)
{
    return b && b->airborne && b->vertical_speed_eighths >= 0;
}

int bfm_castle_drawbridge_actor(const BfmCastleDrawbridge *b, int16_t feet[3],
    int16_t *heading, enum BfmDrawbridgeMotion *motion, unsigned *motion_tick)
{
    if (!b || b->age < PLACE_ACTOR || !feet || !heading || !motion || !motion_tick) return 0;
    memcpy(feet, b->feet, sizeof b->feet);
    *heading = b->heading;
    *motion = b->motion;
    *motion_tick = b->motion_tick;
    return 1;
}

BfmDrawbridgeMusic bfm_castle_drawbridge_music(const BfmCastleDrawbridge *b)
{
    if (!b) return BFM_DRAWBRIDGE_MUSIC_NONE;
    /* The departing room counts ten down through zero before issuing its
     * fade. Selecting the new room cancels any remaining optical audio. */
    if (b->age == 11) return BFM_DRAWBRIDGE_MUSIC_FADE;
    if (b->age == SELECT_CASTLE) return BFM_DRAWBRIDGE_MUSIC_STOP;
    return BFM_DRAWBRIDGE_MUSIC_NONE;
}

int bfm_castle_drawbridge_uses_departure(const BfmCastleDrawbridge *b)
{
    return b && b->age < SELECT_CASTLE;
}

unsigned bfm_castle_drawbridge_shade(const BfmCastleDrawbridge *b)
{
    if (!b) return 31;
    unsigned t = b->age;
    if (t < 14) return 0;
    if (t < SELECT_CASTLE) return (t - 14) * 2;
    if (t <= REVEAL_START + 1) return 31;
    if (t < FULL_LIGHT) return (FULL_LIGHT - t) * 31 / (FULL_LIGHT - REVEAL_START - 1);
    return 0;
}

int bfm_castle_drawbridge_draw(BfmCastleDrawbridge *b,
    const BfmForestPose *pose, const BfmForestTransform *root, uint32_t visible_parts,
    uint32_t pixels[240][320])
{
    if (!b || b->age < REVEAL_START || !pixels) return 0;
    return bfm_castle_drawbridge_view_draw(b->view, b->age - REVEAL_START,
        pose, root, visible_parts, pixels);
}

int bfm_castle_drawbridge_take_boss_request(BfmCastleDrawbridge *b,
    RoomTransferDescriptor *destination)
{
    if (!b || !destination || b->age < RELEASE_GATE || b->feet[2] != GATE_Z || b->request_taken)
        return 0;
    *destination = b->exit;
    b->request_taken = 1;
    return 1;
}

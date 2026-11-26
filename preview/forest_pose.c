/* SPDX-License-Identifier: MIT */
#include "forest_pose.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>

enum { FULL_TURN = 4096, PACKED_POSE = 240 };

static int multiply(int a, int b)
{
    int32_t result = a * b;
    return result / FULL_TURN - (result % FULL_TURN < 0);
}

static void axis_angle(unsigned units, int *s, int *c)
{
    const double radians = 6.28318530717958647693 / FULL_TURN;
    unsigned quadrant = (units / 1024) & 3, phase = units & 1023;
    /* Pose packing mirrors two 1024-entry quarter waves, including their
     * shared endpoints. This affects near-axis bones even in an idle pose. */
    int direct = (int)lround(FULL_TURN * sin(phase * radians));
    int mirror = (int)lround(FULL_TURN * sin((1023-phase) * radians));
    *s = (quadrant & 1) ? mirror : direct;
    *c = (quadrant & 1) ? direct : mirror;
    if (quadrant >= 2) *s = -*s;
    if (quadrant == 1 || quadrant == 2) *c = -*c;
}

void bfm_forest_actor_root(unsigned yaw, const int16_t position[3], BfmForestTransform *root)
{
    int s, c;
    axis_angle(yaw, &s, &c);
    *root = (BfmForestTransform){.rotation = {{c,0,s},{0,FULL_TURN,0},{-s,0,c}},
        .translation = {position[0], position[1], position[2]}};
}

void bfm_forest_actor_pitch_yaw(unsigned pitch, unsigned yaw, const int16_t position[3],
                                BfmForestTransform *root)
{
    int sx, cx, sy, cy;
    axis_angle(pitch, &sx, &cx);
    axis_angle(yaw, &sy, &cy);
    *root = (BfmForestTransform){.rotation = {
        {cy, multiply(sy, sx), multiply(sy, cx)},
        {0, cx, -sx},
        {-sy, multiply(cy, sx), multiply(cy, cx)}},
        .translation = {position[0], position[1], position[2]}};
}

static void local_rotation(const uint8_t *angles, int16_t r[3][3])
{
    int sx, cx, sy, cy, sz, cz;
    axis_angle(angles[6] | (unsigned)angles[7] << 8, &sx, &cx);
    axis_angle(((angles[0] & 15u) << 8) | angles[1], &sy, &cy);
    axis_angle(((angles[0] & 240u) << 4) | angles[2], &sz, &cz);
    /* Euler angles rotate a model vector about X, Y, then Z. The signed
     * products are rounded down to Q12 between successive terms. */
    int xz = multiply(cx, sz), zz = multiply(cx, cz), xy = multiply(sx, sy);
    r[0][0] = (int16_t)multiply(cz, cy);
    r[1][0] = (int16_t)multiply(sz, cy);
    r[2][0] = (int16_t)-sy;
    r[0][1] = (int16_t)(multiply(xy, cz) - xz);
    r[1][1] = (int16_t)(multiply(xy, sz) + zz);
    r[2][1] = (int16_t)multiply(cy, sx);
    r[0][2] = (int16_t)(multiply(zz, sy) + multiply(sx, sz));
    r[1][2] = (int16_t)(multiply(xz, sy) - multiply(sx, cz));
    r[2][2] = (int16_t)multiply(cy, cx);
}

int bfm_forest_actor_pose_decode(const uint8_t *bytes, size_t size, unsigned part_count,
                                 BfmForestPose *pose)
{
    if (!bytes || !pose || !part_count || part_count > MUSASHI_FOREST_PLAYER_PARTS ||
        part_count > size / 8) return 0;
    BfmForestPose decoded = {0};
    for (unsigned i = 0; i < part_count; ++i) {
        const uint8_t *record = bytes + 8 * i;
        local_rotation(record, decoded.local[i].rotation);
        for (unsigned axis = 0; axis < 3; ++axis)
            decoded.local[i].translation[axis] = (int8_t)record[3 + axis];
    }
    *pose = decoded;
    return 1;
}

static int turn_root(int16_t r[3][3])
{
    for (unsigned col = 0; col < 3; ++col) {
        int squared = 0;
        for (unsigned row = 0; row < 3; ++row)
            squared += r[row][col] * r[row][col];
        if (squared < 0xff0000 || squared >= 0x1040000)
            return 0;
        int step = squared < 0x1000000 ? 0x10000 : 0x40000;
        unsigned length = (unsigned)(squared / step * step);
        int gain = (int)sqrt(281474976710656.0 / length);
        for (unsigned row = 0; row < 3; ++row)
            r[row][col] = (int16_t)multiply((col == 1 ? 1 : -1) * r[row][col], gain);
    }
    return 1;
}

int bfm_forest_pose_load(const MusashiDiscMedia *disc, uint32_t frame_address,
                         BfmForestPose *pose)
{
    ForestPlayerArchive archive;
    uint8_t bytes[PACKED_POSE];
    if (!disc || !pose || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, frame_address, bytes, sizeof(bytes)))
        return 0;
    return bfm_forest_pose_decode(bytes, sizeof bytes, pose);
}

int bfm_forest_pose_decode(const uint8_t *bytes, size_t size, BfmForestPose *pose)
{
    if (!bytes || size < PACKED_POSE || !pose)
        return 0;
    BfmForestPose decoded;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        const uint8_t *record = bytes + (i ? (i-1)*8 : 0);
        const uint8_t *xyz = record + (i ? 3 : 0);
        const uint8_t *angles = i < 2 ? bytes + 24 : record;
        BfmForestTransform *part = &decoded.local[i];
        for (unsigned k = 0; k < 3; ++k)
            part->translation[k] = (int8_t)xyz[k];
        local_rotation(angles, part->rotation);
        if (i == 0 && !turn_root(part->rotation))
            return 0;
    }
    *pose = decoded;
    return 1;
}

enum {
    RUN_SLOT = 8, JUMP_SLOT = 13, LAND_SLOT = 14,
    STRIDE_AIR_SLOT = 15, STRIDE_LAND_SLOT = 16, CLIP_WORDS = 32
};

static int read_clip(const ForestPlayerArchive *archive, uint32_t table, unsigned slot,
                     uint8_t *bytes, size_t size)
{
    uint8_t pointer[4];
    if ((table & 3) || table > UINT32_MAX - slot * 4 ||
        !forest_player_archive_read(archive, table + slot * 4, pointer, sizeof pointer))
        return 0;
    uint32_t program = u32le(pointer);
    return !(program & 3) && forest_player_archive_read(archive, program, bytes, size);
}

static int jump_keys(const ForestPlayerArchive *archive, const uint8_t *clip, uint32_t model,
                     enum BfmForestJumpPhase phase, BfmForestJumpPoses *out)
{
    if (u32le(clip) != 4 || u32le(clip + 4) != model || u32le(clip + 8) != 19)
        return 0;
    unsigned interval = 1;
    unsigned falling_keys[CLIP_WORDS] = {0};
    for (unsigned pc = 3; pc < CLIP_WORDS; ++pc) {
        uint32_t command = u32le(clip + 4 * pc);
        unsigned used = out->count[phase];
        switch (command) {
        case 0:
            return phase == BFM_FOREST_JUMP_LANDING && used;
        case 2: /* The rising pose waits here for the action to begin falling. */
            if (phase != BFM_FOREST_JUMP_RISING || !used) return 0;
            phase = BFM_FOREST_JUMP_FALLING;
            break;
        case 3:
            if (++pc >= CLIP_WORDS) return 0;
            interval = u32le(clip + 4 * pc);
            if (!interval || interval > BFM_FOREST_JUMP_TICKS) return 0;
            break;
        case 6: { /* The tail repeats while the actor is still airborne. */
            if (phase != BFM_FOREST_JUMP_FALLING || ++pc >= CLIP_WORDS) return 0;
            uint32_t target = u32le(clip + 4 * pc);
            if (target >= pc || !falling_keys[target]) return 0;
            out->falling_loop = falling_keys[target] - 1;
            return 1;
        }
        case 7: /* Takeoff marker. It occupies no animation tick. */
            if (phase != BFM_FOREST_JUMP_RISING) return 0;
            break;
        default: {
            uint8_t payload[256];
            if ((command & 3) || command > UINT32_MAX - sizeof payload ||
                !forest_player_archive_read(archive, command, payload, sizeof payload) ||
                used > BFM_FOREST_JUMP_TICKS - interval)
                return 0;
            if (phase == BFM_FOREST_JUMP_FALLING) falling_keys[pc] = used + 1;
            for (unsigned i = 0; i < interval; ++i)
                out->frames[phase][used++] = command + 16;
            out->count[phase] = used;
            break;
        }
        }
    }
    return 0;
}

static int load_air_poses(const MusashiDiscMedia *disc, uint32_t table, unsigned air_slot,
                          unsigned land_slot, BfmForestJumpPoses *out)
{
    ForestPlayerArchive archive;
    uint8_t run[12], clip[4 * CLIP_WORDS], parts[4 * (MUSASHI_FOREST_PLAYER_PARTS + 1)];
    BfmForestJumpPoses poses = {0};
    if (!out || !forest_player_archive_open(disc, &archive) ||
        !read_clip(&archive, table, RUN_SLOT, run, sizeof run) ||
        u32le(run) != 4 || u32le(run + 8) != 19)
        return 0;
    uint32_t model = u32le(run + 4);
    if ((model & 3) || !forest_player_archive_read(&archive, model, parts, sizeof parts) ||
        u32le(parts + 4 * MUSASHI_FOREST_PLAYER_PARTS))
        return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if ((u32le(parts + i * 4) >> 24) != 0x80) return 0;
        poses.visible |= UINT32_C(1) << i;
    }
    /* Both clips restore the same equipped Fusion body as the running clip. */
    poses.visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 24) | (UINT32_C(1) << 25));
    if (!read_clip(&archive, table, air_slot, clip, sizeof clip) ||
        !jump_keys(&archive, clip, model, BFM_FOREST_JUMP_RISING, &poses) ||
        !read_clip(&archive, table, land_slot, clip, sizeof clip) ||
        !jump_keys(&archive, clip, model, BFM_FOREST_JUMP_LANDING, &poses))
        return 0;
    *out = poses;
    return 1;
}

int bfm_forest_jump_poses_load(const MusashiDiscMedia *disc, uint32_t table,
                               BfmForestJumpPoses *out)
{
    return load_air_poses(disc, table, JUMP_SLOT, LAND_SLOT, out);
}

int bfm_forest_walkoff_poses_load(const MusashiDiscMedia *disc, uint32_t table,
                                  BfmForestJumpPoses *out)
{
    return load_air_poses(disc, table, STRIDE_AIR_SLOT, STRIDE_LAND_SLOT, out);
}

uint32_t bfm_forest_walkoff_pose_frame(const BfmForestJumpPoses *poses, unsigned airborne_tick)
{
    /* The entry sequence ends at a hold. Keep that pose while the actor is
     * unsupported; touching down selects a different mounted clip. */
    return bfm_forest_jump_pose_frame(poses, BFM_FOREST_JUMP_RISING, airborne_tick);
}

uint32_t bfm_forest_jump_pose_frame(const BfmForestJumpPoses *poses,
                                    enum BfmForestJumpPhase phase, unsigned phase_tick)
{
    if (!poses || (unsigned)phase > BFM_FOREST_JUMP_LANDING) return 0;
    unsigned count = poses->count[phase];
    if (!count || count > BFM_FOREST_JUMP_TICKS) return 0;
    /* The action changes the model two updates after the vertical velocity
     * turns downward. The jump program stays on its suspended rise key
     * during those updates, including after a short button press. */
    if (phase == BFM_FOREST_JUMP_FALLING) {
        if (phase_tick < 2) {
            unsigned rise_count = poses->count[BFM_FOREST_JUMP_RISING];
            if (!rise_count || rise_count > BFM_FOREST_JUMP_TICKS) return 0;
            return poses->frames[BFM_FOREST_JUMP_RISING][rise_count - 1];
        }
        phase_tick -= 2;
    }
    if (phase_tick >= count) {
        if (phase == BFM_FOREST_JUMP_FALLING) {
            unsigned loop = poses->falling_loop;
            if (loop >= count) return 0;
            phase_tick = loop + (phase_tick - loop) % (count - loop);
        } else {
            phase_tick = count - 1;
        }
    }
    return poses->frames[phase][phase_tick];
}

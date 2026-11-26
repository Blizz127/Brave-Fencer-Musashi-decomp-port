/* SPDX-License-Identifier: MIT */
#include "steam_knight_machine.h"
#include "forest_player_bytes.h"
#include <stdlib.h>

enum { CHASSIS, TORSO, ASSEMBLIES, MAX_KEYS = 16 };
enum { NEUTRAL, FIRST_ACTION, LOWER, RISE, CLIPS };
static const uint32_t origin = UINT32_C(0x80128158);
static const struct { uint32_t table; unsigned parts; } model[] = {
    {0x801b82f4, 22}, {0x801bb8dc, 10}
};
static const struct { uint32_t table; unsigned index, keys; } chassis_program[] = {
    {0x801945b0, 0, 16}, {0x80194960, 6, 8}, {0x801949a8, 7, 5}, {0x801949d8, 8, 8}
};
typedef struct {
    BfmForestPose pose[MAX_KEYS];
    unsigned count, terminal;
} MachineClip;
struct BfmSteamKnightMachine {
    MusashiForestPlayerMesh *mesh[ASSEMBLIES];
    MachineClip chassis[CLIPS], torso;
    BfmForestTransform root, display_root;
    const BfmForestPose *display[ASSEMBLIES], *chassis_pose;
    unsigned entrance_age, outgoing_display;
    int spawn_y, active, controlled;
};

static const uint8_t *record(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < origin || (address & 3) || count > bytes || address - origin > bytes - count)
        return NULL;
    return scene + (address - origin);
}

static int decode_pose(const uint8_t *frame, unsigned parts, BfmForestPose *pose)
{
    /* Full joint records contain signed XYZ followed by X/Y/Z angles. Use
     * the field renderer's existing fixed-point Euler conversion. */
    uint8_t angles[8 * MUSASHI_FOREST_PLAYER_PARTS] = {0};
    for (unsigned i = 0; i < parts; ++i) {
        const uint8_t *joint = frame + 12 * i;
        uint8_t *packed = angles + 8 * i;
        uint16_t y = u16le(joint + 8), z = u16le(joint + 10);
        packed[0] = (uint8_t)(((z >> 4) & 0xf0) | ((y >> 8) & 15));
        packed[1] = (uint8_t)y;
        packed[2] = (uint8_t)z;
        packed[6] = joint[6];
        packed[7] = joint[7];
    }
    if (!bfm_forest_actor_pose_decode(angles, sizeof angles, parts, pose))
        return 0;
    for (unsigned i = 0; i < parts; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            pose->local[i].translation[axis] = s16le(frame + 12 * i + 2 * axis);
    return 1;
}

static int load_clip(MachineClip *clip, const uint8_t *scene, size_t bytes, unsigned piece,
                     uint32_t table, unsigned count)
{
    const uint8_t *program = record(scene, bytes, table, 8 * (count + 1));
    if (!program || !count || count > MAX_KEYS || u32le(program + 8 * count)) return 0;
    unsigned terminal = u32le(program + 8 * count + 4);
    if (terminal != 0xfffe && terminal != 0xffff) return 0;
    unsigned stride = 12 * model[piece].parts;
    uint32_t first = piece == CHASSIS ? UINT32_C(0x801bdcd8) : UINT32_C(0x801d323c);
    uint32_t last = piece == CHASSIS ? UINT32_C(0x801d3128) : UINT32_C(0x801d3944);
    for (unsigned i = 0; i < count; ++i) {
        uint32_t key = u32le(program + 8*i);
        if (key < first || key > last || (key - first) % stride ||
            u32le(program + 8*i + 4) != 2) return 0;
        const uint8_t *frame = record(scene, bytes, key, stride);
        if (!frame || !decode_pose(frame, model[piece].parts, &clip->pose[i])) return 0;
    }
    clip->count = count;
    clip->terminal = terminal;
    return 1;
}
BfmSteamKnightMachine *bfm_steam_knight_machine_load(const uint8_t *scene, size_t bytes)
{
    if (!scene) return NULL;
    const uint8_t *placement = record(scene, bytes, UINT32_C(0x801976d4), 60);
    if (!placement || u16le(placement + 6) != 61 || u16le(placement + 26) != 116 ||
        u16le(placement + 46)) return NULL;
    int16_t at[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        at[axis] = s16le(placement + 2*axis);
        if (at[axis] != s16le(placement + 20 + 2*axis)) return NULL;
    }
    BfmSteamKnightMachine *machine = calloc(1, sizeof *machine);
    if (!machine) return NULL;
    bfm_forest_actor_root(0, at, &machine->root);
    machine->spawn_y = at[1];
    machine->display_root = machine->root;
    for (unsigned piece = 0; piece < ASSEMBLIES; ++piece) {
        unsigned count = 0;
        machine->mesh[piece] = musashi_forest_player_mesh_from_table(
            scene, bytes, origin, model[piece].table, &count);
        if (!machine->mesh[piece] || count != model[piece].parts) goto failed;
    }
    for (unsigned i = 0; i < CLIPS; ++i)
        if (!load_clip(&machine->chassis[i], scene, bytes, CHASSIS,
                       chassis_program[i].table, chassis_program[i].keys)) goto failed;
    if (!load_clip(&machine->torso, scene, bytes, TORSO, UINT32_C(0x801950d0), 16)) goto failed;
    return machine;
failed:
    bfm_steam_knight_machine_free(machine);
    return NULL;
}

void bfm_steam_knight_machine_free(BfmSteamKnightMachine *machine)
{
    if (!machine) return;
    for (unsigned i = 0; i < ASSEMBLIES; ++i)
        musashi_forest_player_mesh_free(machine->mesh[i]);
    free(machine);
}

static const BfmForestPose *sample(const MachineClip *clip, unsigned elapsed)
{
    unsigned key = elapsed / 2;
    if (clip->terminal == 0xffff) key %= clip->count;
    else if (key >= clip->count) key = clip->count - 1;
    return &clip->pose[key];
}

/* A capped acceleration fits both measured points during the drop and the
 * shoulder's motion across the open ceiling. The first five updates precede
 * this camera cue. The intermediate trajectory is an optical estimate. */
static int fall_height(int start, unsigned age)
{
    if (age >= 29) return -2050;
    int drop = age <= 23 ? (int)(age * (age + 1)) : 552 + (int)(age - 23) * 46;
    return start + drop < -2050 ? start + drop : -2050;
}

/* The impact slows the cycle before the steady two-update key cadence
 * resumes in the camera pan. */
static unsigned entrance_clock(unsigned age)
{
    if (age < 24) return 13 + age;
    if (age < 78) return 36 + (age - 24) * 2 / 3;
    return (age - 6) % 32;
}

void bfm_steam_knight_machine_present(BfmSteamKnightMachine *machine, unsigned elapsed)
{
    if (!machine) return;
    unsigned clock, shown;
    if (machine->controlled) {
        clock = elapsed % 32;
        shown = elapsed < 2 ? machine->outgoing_display : (elapsed - 2) % 32;
        machine->root.translation[1] = machine->display_root.translation[1] = -2050;
    } else {
        machine->entrance_age = elapsed;
        clock = entrance_clock(elapsed);
        shown = (clock + 30) % 32;
        unsigned age = elapsed > 29 ? 29 : elapsed;
        machine->root.translation[1] = fall_height(machine->spawn_y, age + 5);
        machine->display_root.translation[1] = fall_height(machine->spawn_y, age + 2);
    }
    machine->active = 1;
    machine->chassis_pose = sample(&machine->chassis[NEUTRAL], clock);
    machine->display[CHASSIS] = sample(&machine->chassis[NEUTRAL], shown);
    machine->display[TORSO] = sample(&machine->torso, shown);
}

void bfm_steam_knight_machine_handover(BfmSteamKnightMachine *machine)
{
    if (!machine || !machine->active || machine->controlled) return;
    /* The first issued control update retains the outgoing camera submission. */
    machine->outgoing_display = (entrance_clock(machine->entrance_age) + 31) % 32;
    machine->controlled = 1;
    bfm_steam_knight_machine_present(machine, 0);
}

int bfm_steam_knight_machine_set_chassis_clip(BfmSteamKnightMachine *machine,
                                             unsigned clip, unsigned elapsed)
{
    if (!machine || !machine->controlled) return 0;
    for (unsigned i = 0; i < CLIPS; ++i)
        if (chassis_program[i].index == clip) {
            machine->chassis_pose = sample(&machine->chassis[i], elapsed);
            if (elapsed >= 2)
                machine->display[CHASSIS] = sample(&machine->chassis[i], elapsed - 2);
            return 1;
        }
    return 0;
}

size_t bfm_steam_knight_machine_actors(const BfmSteamKnightMachine *machine,
                                      BfmForestActor *out, size_t capacity)
{
    if (!machine || !machine->active || !out || capacity < ASSEMBLIES) return 0;
    for (unsigned i = 0; i < ASSEMBLIES; ++i)
        out[i] = (BfmForestActor){.mesh = machine->mesh[i], .pose = machine->display[i],
            .root = &machine->display_root, .visible_parts = (UINT32_C(1) << model[i].parts) - 1};
    return ASSEMBLIES;
}

int bfm_steam_knight_machine_joint(const BfmSteamKnightMachine *machine, unsigned chassis_part,
                                  BfmForestTransform *world, const MusashiForestPlayerPart **mesh_part)
{
    if (!machine || !machine->active || chassis_part >= model[CHASSIS].parts || !world || !mesh_part)
        return 0;
    /* All lower joints use the same unit-scale, zero-yaw world transform. */
    *world = machine->chassis_pose->local[chassis_part];
    for (unsigned axis = 0; axis < 3; ++axis)
        world->translation[axis] += machine->root.translation[axis];
    *mesh_part = musashi_forest_player_mesh_part(machine->mesh[CHASSIS], chassis_part);
    return *mesh_part != NULL;
}

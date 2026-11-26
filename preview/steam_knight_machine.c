/* SPDX-License-Identifier: MIT */
#include "steam_knight_machine.h"
#include "forest_dust.h"
#include <math.h>
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { CHASSIS, TORSO, ASSEMBLIES, MAX_KEYS = 24 };
enum { NEUTRAL, FIRST_ACTION, LOWER, RISE, STEP_RIGHT, STEP_LEFT, STEP_FORWARD,
       KNEEL_RIGHT, KNEEL_LEFT, SMASH, SMASH_RISE, COLLAPSE, DEPARTURE, ARRIVAL, REARM, JUMP_START, AIRBORNE, LANDED, CLIPS };

enum { GAUNTLET = 6, FIRST_HIT_JOINT = 18, FIRST_WHITE_UPDATE = 2, OBSERVED_WHITE_UPDATES = 6 };
static const uint32_t origin = UINT32_C(0x80128158);
static const struct { uint32_t table; unsigned parts; } model[] = {
    {0x801b82f4, 22}, {0x801bb8dc, 10}
};
static const struct { uint32_t table; unsigned index, keys; } chassis_program[] = {
    [NEUTRAL] = {0x801945b0, 0, 16}, [FIRST_ACTION] = {0x80194960, 6, 8},
    [STEP_RIGHT] = {0x80194638, 1, 24}, [STEP_LEFT] = {0x80194700, 2, 24},
    [STEP_FORWARD] = {0x801947c8, 3, 24},
    [KNEEL_RIGHT] = {0x80194ea8, 20, 16}, [KNEEL_LEFT] = {0x80194f78, 22, 16},
    [COLLAPSE] = {0x80194ae0, 11, 5}, [DEPARTURE] = {0x80194a20, 9, 17},
    [ARRIVAL] = {0x80194b10, 12, 17}, [REARM] = {0x80194de0, 19, 24},
    [JUMP_START] = {0x80194c40, 15, 2}, [AIRBORNE] = {0x80194c58, 16, 14},
    [LANDED] = {0x80194cd0, 17, 15},
    [SMASH] = {0x80194ba0, 13, 12}, [SMASH_RISE] = {0x80194c08, 14, 6},
    [LOWER]   = {0x801949a8, 7, 5},  [RISE] = {0x801949d8, 8, 8}
};
typedef struct {
    BfmForestPose *pose;
    unsigned count, terminal;
} MachineClip;
struct BfmSteamKnightMachine {
    MusashiForestPlayerMesh *mesh[ASSEMBLIES], *white_chassis, *under_armor;
    MusashiForestPlayerMesh *lit_mesh[ASSEMBLIES], *lit_under_armor;
    unsigned core_light;
    uint64_t shell_white_from, shell_white_until;
    MachineClip chassis[CLIPS], torso;
    BfmForestTransform root, display_root;
    const BfmForestPose *display[ASSEMBLIES], *chassis_pose;
    unsigned entrance_age, outgoing_display, control_age, clip_epoch, clip_age;
    const MachineClip *selected;
    const BfmForestPose *outgoing[2];
    BfmForestPose mounted_torso, armor_pose;
    uint32_t broken_shells;
    uint64_t shell_change_at[4];
    unsigned opening_attacks;
    int knee_seen, new_break_in_strike, walk_after_strike, open_cover;
    BfmSteamKnightStance action;
    int step_x, step_z, patrol_after_recovery;
    unsigned neutral_since, round, round_stage, phase_age, yaw, y_fraction;
    const BfmForestPose *jump_now, *jump_shown, *previous_jump_pose;
    int32_t x4, z4, action_x4, action_z4;
    int root_remainder[3];
    unsigned shot_windup, shot_release;
    int shot_recovering;
    int spawn_y, active, revealed, controlled;
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
    clip->pose = calloc(count, sizeof *clip->pose);
    if (!clip->pose) return 0;
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
    machine->x4 = at[0] * 4;
    machine->z4 = at[2] * 4;
    machine->display_root = machine->root;
    for (unsigned piece = 0; piece < ASSEMBLIES; ++piece) {
        unsigned count = 0;
        machine->mesh[piece] = musashi_forest_player_mesh_from_table(
            scene, bytes, origin, model[piece].table, &count);
        if (!machine->mesh[piece] || count != model[piece].parts) goto failed;
    }
    /* Independently registered original views show the shoulders, torso,
     * head, waist and these leg surfaces brighten. The wrist stays dark. */
    const uint32_t lit_parts[] = {
        (1u << 11) | (1u << 13) | (1u << 14) | (1u << 15) |
        (1u << 16) | (1u << 20) | (1u << 21),
        (1u << 0) | (1u << 3) | (1u << 4) | (1u << 5)
    };
    for (unsigned i = 0; i < ASSEMBLIES; ++i) {
        machine->lit_mesh[i] = musashi_forest_player_mesh_lit_view(machine->mesh[i], lit_parts[i], 96);
        if (!machine->lit_mesh[i]) goto failed;
    }
    unsigned parts = 0;
    machine->under_armor = musashi_forest_player_mesh_from_table(
        scene, bytes, origin, UINT32_C(0x801bd9d4), &parts);
    if (!machine->under_armor || parts != 12) goto failed;
    machine->lit_under_armor = musashi_forest_player_mesh_lit_view(
        machine->under_armor, (1u << 8) | (1u << 10), 96);
    if (!machine->lit_under_armor) goto failed;
    machine->white_chassis = musashi_forest_player_mesh_white_part_view(machine->mesh[CHASSIS], FIRST_HIT_JOINT);
    if (!machine->white_chassis) goto failed;
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
    for (unsigned i = 0; i < CLIPS; ++i) free(machine->chassis[i].pose);
    free(machine->torso.pose);
    musashi_forest_player_mesh_free(machine->lit_under_armor);
    musashi_forest_player_mesh_free(machine->under_armor);
    musashi_forest_player_mesh_free(machine->white_chassis);
    for (unsigned i = 0; i < ASSEMBLIES; ++i) {
        musashi_forest_player_mesh_free(machine->lit_mesh[i]);
        musashi_forest_player_mesh_free(machine->mesh[i]);
    }
    free(machine);
}

static const BfmForestPose *sample(const MachineClip *clip, uint64_t elapsed)
{
    uint64_t key = elapsed / 2;
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

static const BfmForestPose *action_pose(const BfmSteamKnightMachine *machine, unsigned age)
{
    const MachineClip *clip = machine->selected;
    if (machine->action == BFM_STEAM_MACHINE_TRANSFERRING) {
        if (machine->round_stage == 10) {
            const MachineClip *loop = &machine->chassis[COLLAPSE];
            return &loop->pose[(age / 3) % loop->count];
        }
        if (machine->round_stage == 8) return sample(&machine->chassis[DEPARTURE], age);
        if (age < 34) return sample(&machine->chassis[ARRIVAL], age);
        if (machine->round == 2) return &machine->chassis[NEUTRAL].pose[(age - 34) % 16];
        unsigned key = age - 34;
        return &machine->chassis[REARM].pose[key < 24 ? key : 23];
    }
    if (machine->action == BFM_STEAM_MACHINE_STREET_SHOT) {
        if (machine->shot_recovering && age >= machine->shot_release)
            return sample(&machine->chassis[RISE], age - machine->shot_release);
        if (age < machine->shot_windup) return sample(&machine->chassis[FIRST_ACTION], age);
        return sample(&machine->chassis[LOWER], age - machine->shot_windup);
    }
    if (machine->action == BFM_STEAM_MACHINE_STREET_WALKING)
        return &clip->pose[age % clip->count];
    if (machine->action == BFM_STEAM_MACHINE_NEUTRAL && machine->round == 2)
        return &machine->chassis[NEUTRAL].pose[age % 16];
    if (machine->action == BFM_STEAM_MACHINE_STRIKING) {
        if (age < 47) return sample(&machine->chassis[FIRST_ACTION], age);
        if (age < 126) return sample(&machine->chassis[LOWER], age - 47);
        return sample(&machine->chassis[RISE], age - 126);
    }
    if (machine->action == BFM_STEAM_MACHINE_SMASHING) {
        if (age < 168) return sample(&machine->chassis[SMASH], age);
        return sample(&machine->chassis[SMASH_RISE], age - 168);
    }
    if (machine->action == BFM_STEAM_MACHINE_GETTING_UP)
        return &clip->pose[age < clip->count ? clip->count - 1 - age : 0];
    return sample(clip, (uint64_t)machine->clip_age + age);
}

static void select_clip(BfmSteamKnightMachine *machine, unsigned id, unsigned age)
{
    unsigned elapsed = machine->control_age - machine->clip_epoch;
    machine->outgoing[0] = machine->display[CHASSIS];
    if (machine->action == BFM_STEAM_MACHINE_JUMPING) machine->outgoing[1] = machine->chassis_pose;
    else if (machine->selected && elapsed) machine->outgoing[1] = action_pose(machine, elapsed - 1);
    else if (!machine->selected) {
        unsigned tick = machine->control_age;
        machine->outgoing[1] = sample(&machine->chassis[NEUTRAL], tick ? tick - 1 : 0);
    }
    machine->selected = &machine->chassis[id];
    machine->clip_epoch = machine->control_age;
    machine->clip_age = age;
    machine->action_x4 = machine->x4;
    machine->action_z4 = machine->z4;
}

/* Carry the upper assembly on the posed waist. Its neutral reference waist
 * has identity rotation and its position already occurs in each upper joint. */
static void mounted_upper_part(const BfmForestTransform *waist, const BfmForestTransform *reference,
                               const BfmForestTransform *local, BfmForestTransform *placed)
{
    for (unsigned i = 0; i < 3; ++i) {
        int move = 0;
        for (unsigned k = 0; k < 3; ++k)
            move += waist->rotation[i][k] * (local->translation[k] - reference->translation[k]);
        placed->translation[i] = waist->translation[i] + (move >> 12);
        for (unsigned j = 0; j < 3; ++j) {
            int turn = 0;
            for (unsigned k = 0; k < 3; ++k)
                turn += waist->rotation[i][k] * local->rotation[k][j];
            placed->rotation[i][j] = (int16_t)(turn >> 12);
        }
    }
}

static void mount_torso(BfmSteamKnightMachine *machine, unsigned shown)
{
    const BfmForestTransform *waist = &machine->display[CHASSIS]->local[21];
    const BfmForestTransform *reference = &sample(&machine->chassis[NEUTRAL], shown)->local[21];
    const BfmForestPose *source = sample(&machine->torso, shown);
    for (unsigned p = 0; p < model[TORSO].parts; ++p) {
        const BfmForestTransform *local = &source->local[p];
        BfmForestTransform *placed = &machine->mounted_torso.local[p];
        mounted_upper_part(waist, reference, local, placed);
    }
    machine->display[TORSO] = &machine->mounted_torso;
}

static void selected_chassis(BfmSteamKnightMachine *machine, unsigned shown)
{
    if (machine->action == BFM_STEAM_MACHINE_JUMPING) {
        machine->chassis_pose = machine->jump_now;
        machine->display[CHASSIS] = machine->jump_shown;
        mount_torso(machine, shown);
        return;
    }
    if (!machine->selected || machine->control_age < machine->clip_epoch) return;
    unsigned age = machine->control_age - machine->clip_epoch;
    if (machine->action == BFM_STEAM_MACHINE_WALKING) {
        unsigned steps = age < 48 ? age : 48;
        machine->x4 = machine->action_x4 + (int)steps * 7 * machine->step_x;
        machine->z4 = machine->action_z4 + (int)steps * 7 * machine->step_z;
        machine->root.translation[0] = machine->display_root.translation[0] = machine->x4 >> 2;
        machine->root.translation[2] = machine->display_root.translation[2] = machine->z4 >> 2;
    }
    int driven = machine->action == BFM_STEAM_MACHINE_TRANSFERRING ||
                 machine->action == BFM_STEAM_MACHINE_STREET_WALKING ||
                 machine->action == BFM_STEAM_MACHINE_STREET_SHOT;
    if (driven) age = machine->phase_age;
    unsigned duration = machine->action == BFM_STEAM_MACHINE_WALKING ? 48 : machine->selected->count;
    if (machine->action == BFM_STEAM_MACHINE_STRIKING) duration = 142;
    if (machine->action == BFM_STEAM_MACHINE_SMASHING) duration = 180;
    if (machine->action != BFM_STEAM_MACHINE_KNEELING &&
        machine->action != BFM_STEAM_MACHINE_NEUTRAL &&
        !driven && age >= duration) {
        machine->walk_after_strike = machine->action == BFM_STEAM_MACHINE_STRIKING && machine->new_break_in_strike;
        int recovered = machine->action == BFM_STEAM_MACHINE_GETTING_UP;
        unsigned remaining = age - duration;
        machine->outgoing[0] = action_pose(machine, duration - 2);
        machine->outgoing[1] = action_pose(machine, duration - 1);
        machine->action = BFM_STEAM_MACHINE_NEUTRAL;
        machine->selected = &machine->chassis[NEUTRAL];
        machine->clip_epoch = machine->control_age - remaining;
        machine->clip_age = 0;
        machine->neutral_since = machine->clip_epoch;
        machine->patrol_after_recovery = recovered && machine->round == 0;
        age = remaining;
    }
    machine->chassis_pose = action_pose(machine, age);
    machine->display[CHASSIS] = age < 2 ? machine->outgoing[age] : action_pose(machine, age - 2);
    mount_torso(machine, shown);
}

void bfm_steam_knight_machine_present(BfmSteamKnightMachine *machine, unsigned elapsed)
{
    if (!machine) return;
    unsigned clock, shown;
    if (machine->controlled) {
        machine->control_age = elapsed;
        clock = elapsed % 32;
        shown = elapsed < 2 ? machine->outgoing_display : (elapsed - 2) % 32;
    } else {
        machine->entrance_age = elapsed;
        clock = entrance_clock(elapsed);
        shown = (clock + 30) % 32;
        unsigned age = elapsed > 29 ? 29 : elapsed;
        machine->root.translation[1] = fall_height(machine->spawn_y, age + 5);
        int y0 = fall_height(machine->spawn_y, age + 2);
        int y1 = fall_height(machine->spawn_y, age + 3);
        /* The exposed forearm trails the sampled collision assembly. */
        machine->display_root.translation[1] = y0 + (y1 - y0) * 2 / 3;
    }
    machine->active = 1;
    machine->chassis_pose = sample(&machine->chassis[NEUTRAL], clock);
    machine->display[CHASSIS] = sample(&machine->chassis[NEUTRAL], shown);
    machine->display[TORSO] = sample(&machine->torso, shown);
    selected_chassis(machine, shown);
    machine->armor_pose.local[8] = machine->display[CHASSIS]->local[13];
    machine->armor_pose.local[9] = machine->display[CHASSIS]->local[14];
    machine->armor_pose.local[10] = machine->display[CHASSIS]->local[18];
    machine->armor_pose.local[11] = machine->display[CHASSIS]->local[19];
    if (machine->walk_after_strike ||
        (machine->patrol_after_recovery && machine->control_age - machine->neutral_since >= 32))
        bfm_steam_knight_machine_walk(machine, BFM_STEAM_WALK_FORWARD);
}

void bfm_steam_knight_machine_reveal(BfmSteamKnightMachine *machine)
{
    if (machine) machine->revealed = 1;
}

void bfm_steam_knight_machine_handover(BfmSteamKnightMachine *machine)
{
    if (!machine || !machine->active || machine->controlled) return;
    /* The first issued control update retains the outgoing camera submission. */
    machine->outgoing_display = (entrance_clock(machine->entrance_age) + 31) % 32;
    machine->root.translation[1] = machine->display_root.translation[1] = -2050;
    machine->action = BFM_STEAM_MACHINE_NEUTRAL;
    machine->controlled = 1;
    bfm_steam_knight_machine_present(machine, 0);
}

int bfm_steam_knight_machine_set_chassis_clip(BfmSteamKnightMachine *machine,
                                             unsigned clip, unsigned elapsed)
{
    if (!machine || !machine->controlled || machine->action != BFM_STEAM_MACHINE_NEUTRAL) return 0;
    for (unsigned i = 0; i < CLIPS; ++i) {
        if (chassis_program[i].index != clip) continue;
        select_clip(machine, i, elapsed);
        bfm_steam_knight_machine_present(machine, machine->control_age);
        return 1;
    }
    return 0;
}

int bfm_steam_knight_machine_walk(BfmSteamKnightMachine *machine, BfmSteamKnightWalk direction)
{
    if (!machine || !machine->controlled || machine->action != BFM_STEAM_MACHINE_NEUTRAL ||
        (unsigned)direction > BFM_STEAM_WALK_FORWARD) return 0;
    unsigned id = direction == BFM_STEAM_WALK_FORWARD ? STEP_FORWARD :
                  direction == BFM_STEAM_WALK_RIGHT ? STEP_RIGHT : STEP_LEFT;
    select_clip(machine, id, 0);
    machine->step_x = (direction == BFM_STEAM_WALK_RIGHT) - (direction == BFM_STEAM_WALK_LEFT);
    machine->step_z = direction == BFM_STEAM_WALK_FORWARD;
    machine->action = BFM_STEAM_MACHINE_WALKING;
    machine->walk_after_strike = 0;
    machine->patrol_after_recovery = 0;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    return 1;
}

BfmSteamKnightStance bfm_steam_knight_machine_stance(const BfmSteamKnightMachine *machine)
{
    return machine && machine->controlled ? machine->action : BFM_STEAM_MACHINE_DORMANT;
}

int bfm_steam_knight_machine_world_root(const BfmSteamKnightMachine *machine, int16_t position[3])
{
    if (!machine || !machine->active || !position) return 0;
    for (unsigned i = 0; i < 3; ++i) position[i] = (int16_t)machine->root.translation[i];
    return 1;
}

int bfm_steam_knight_machine_set_world_root(BfmSteamKnightMachine *machine, const int16_t position[3])
{
    if (!machine || !machine->controlled || !position) return 0;
    machine->action_x4 += 4 * position[0] - machine->x4;
    machine->action_z4 += 4 * position[2] - machine->z4;
    for (unsigned i = 0; i < 3; ++i) machine->root_remainder[i] = 0;
    machine->y_fraction = 0;
    machine->x4 = 4 * position[0];
    machine->z4 = 4 * position[2];
    for (unsigned i = 0; i < 3; ++i)
        machine->root.translation[i] = machine->display_root.translation[i] = position[i];
    return 1;
}

/* The observed first hall makes its choices at the end of a neutral cycle.
 * Only the opening approach, flank and high airborne responses are bracketed
 * by repeated player runs. Later hall choices have additional unknown inputs. */
void bfm_steam_knight_machine_track_hero(BfmSteamKnightMachine *machine, const int16_t feet[3])
{
    if (!machine || !feet || !machine->controlled || machine->action != BFM_STEAM_MACHINE_NEUTRAL ||
        machine->control_age < machine->neutral_since) return;
    unsigned elapsed = machine->control_age - machine->neutral_since;
    if (!elapsed || elapsed % 32) return;
    int dx = feet[0] - machine->root.translation[0];
    int dy = feet[1] - machine->root.translation[1];
    int dz = feet[2] - machine->root.translation[2];
    int flank = abs(dx);
    if (flank >= 144 && flank <= 192 && dz >= -256 && dz <= -128 &&
        (machine->knee_seen || elapsed >= 96)) {
        bfm_steam_knight_machine_walk(machine, dx < 0 ? BFM_STEAM_WALK_LEFT : BFM_STEAM_WALK_RIGHT);
        return;
    }
    if (machine->knee_seen || dx > 0 || dx < -128 || dz < -384 || dz > -128) return;
    if (machine->opening_attacks < 2) {
        unsigned first_cycle = machine->opening_attacks ? 96 : 128;
        if (elapsed < first_cycle) return;
        select_clip(machine, FIRST_ACTION, 0);
        machine->action = BFM_STEAM_MACHINE_STRIKING;
    } else {
        if (machine->opening_attacks != 2 || dy > -256 || dx < -64 || dz > -256) return;
        select_clip(machine, SMASH, 0);
        machine->action = BFM_STEAM_MACHINE_SMASHING;
    }
    machine->new_break_in_strike = 0;
    ++machine->opening_attacks;
    bfm_steam_knight_machine_present(machine, machine->control_age);
}

/* The outer cap remains visible for two submitted updates after contact.
 * The inner collar is also visible in the first two outputs after its hit. */
static int break_shell(BfmSteamKnightMachine *machine, unsigned joint, unsigned slot)
{
    uint32_t bit = UINT32_C(1) << joint;
    if (machine->broken_shells & bit) return 0;
    machine->broken_shells |= bit;
    machine->shell_change_at[slot - 8] = (uint64_t)machine->control_age + 2;
    return 1;
}

int bfm_steam_knight_machine_outer_broken(BfmSteamKnightMachine *machine, unsigned joint)
{
    if (!machine || !machine->controlled || (joint != 13 && joint != 18)) return 0;
    if (!break_shell(machine, joint, joint == 13 ? 8 : 10)) return 0;
    if (machine->action == BFM_STEAM_MACHINE_STRIKING && !machine->knee_seen) machine->new_break_in_strike = 1;
    return 1;
}

int bfm_steam_knight_machine_inner_broken(BfmSteamKnightMachine *machine, unsigned joint)
{
    if (!machine || !machine->controlled || (joint != 14 && joint != 19) ||
        machine->action == BFM_STEAM_MACHINE_KNEELING || machine->action == BFM_STEAM_MACHINE_GETTING_UP) return 0;
    if (!break_shell(machine, joint, joint == 14 ? 9 : 11)) return 0;
    select_clip(machine, joint == 14 ? KNEEL_RIGHT : KNEEL_LEFT, 0);
    machine->action = BFM_STEAM_MACHINE_KNEELING;
    machine->knee_seen = 1;
    machine->patrol_after_recovery = 0;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    return 1;
}

int bfm_steam_knight_machine_recover(BfmSteamKnightMachine *machine)
{
    if (!machine || machine->action != BFM_STEAM_MACHINE_KNEELING ||
        machine->control_age - machine->clip_epoch < 2 * (machine->selected->count - 1)) return 0;
    unsigned id = machine->selected == &machine->chassis[KNEEL_RIGHT] ? KNEEL_RIGHT : KNEEL_LEFT;
    select_clip(machine, id, 0);
    machine->action = BFM_STEAM_MACHINE_GETTING_UP;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    return 1;
}

size_t bfm_steam_knight_machine_actors(const BfmSteamKnightMachine *machine,
                                      BfmForestActor *out, size_t capacity)
{
    if (!machine || !machine->active || !machine->revealed || !out || capacity < ASSEMBLIES) return 0;
    for (unsigned i = 0; i < ASSEMBLIES; ++i)
        out[i] = (BfmForestActor){.mesh = machine->mesh[i], .pose = machine->display[i],
            .root = &machine->display_root, .visible_parts = (UINT32_C(1) << model[i].parts) - 1};
    if (machine->control_age >= machine->shell_white_from && machine->control_age < machine->shell_white_until)
        out[CHASSIS].mesh = machine->white_chassis;
    int lit = machine->core_light && machine->core_light <= 7;
    if (lit)
        for (unsigned i = 0; i < ASSEMBLIES; ++i) out[i].mesh = machine->lit_mesh[i];
    if (machine->open_cover) out[CHASSIS].visible_parts &= ~UINT32_C(1);
    if (machine->broken_shells && capacity > ASSEMBLIES) {
        static const unsigned joints[] = {13, 14, 18, 19};
        uint32_t slots = 0;
        for (unsigned i = 0; i < 4; ++i) {
            uint32_t part = UINT32_C(1) << joints[i];
            if ((machine->broken_shells & part) && machine->control_age >= machine->shell_change_at[i]) {
                slots |= UINT32_C(1) << (8 + i);
                out[CHASSIS].visible_parts &= ~part;
            }
        }
        if (!slots) return ASSEMBLIES;
        out[ASSEMBLIES] = (BfmForestActor){.mesh = lit ? machine->lit_under_armor : machine->under_armor,
            .pose = &machine->armor_pose, .root = &machine->display_root, .visible_parts = slots};
        return ASSEMBLIES + 1;
    }
    return ASSEMBLIES;
}

size_t bfm_steam_knight_machine_gauntlet_actors(const BfmSteamKnightMachine *machine,
    const BfmForestTransform *detached_world, BfmForestActor *out, size_t capacity)
{
    static const BfmForestPose origin = {.local = {
        [GAUNTLET] = {.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}}
    }};
    if (!detached_world) return bfm_steam_knight_machine_actors(machine, out, capacity);
    if (capacity < ASSEMBLIES + 1) return 0;
    size_t count = bfm_steam_knight_machine_actors(machine, out, capacity);
    if (!count || count >= capacity) return 0;
    uint32_t bit = UINT32_C(1) << GAUNTLET;
    out[TORSO].visible_parts &= ~bit;
    out[count] = (BfmForestActor){.mesh = machine->mesh[TORSO], .pose = &origin,
        .root = detached_world, .visible_parts = bit};
    return count + 1;
}

void bfm_steam_knight_machine_step_light(BfmSteamKnightMachine *machine)
{
    if (machine && machine->core_light) --machine->core_light;
}

void bfm_steam_knight_machine_flash_core(BfmSteamKnightMachine *machine)
{
    if (machine && machine->controlled) machine->core_light = 9;
}

void bfm_steam_knight_machine_flash_armor(BfmSteamKnightMachine *machine, unsigned joint)
{
    if (!machine || !machine->controlled || joint != FIRST_HIT_JOINT) return;
    machine->shell_white_from = (uint64_t)machine->control_age + FIRST_WHITE_UPDATE;
    machine->shell_white_until = machine->shell_white_from + OBSERVED_WHITE_UPDATES;
}

static void world_part(const BfmSteamKnightMachine *machine, const BfmForestTransform *local,
                       BfmForestTransform *world)
{
    *world = *local;
    for (unsigned i = 0; i < 3; ++i) {
        if (machine->yaw) {
            int move = 0;
            for (unsigned k = 0; k < 3; ++k)
                move += machine->root.rotation[i][k] * local->translation[k];
            world->translation[i] = move >> 12;
            for (unsigned j = 0; j < 3; ++j) {
                int turn = 0;
                for (unsigned k = 0; k < 3; ++k)
                    turn += machine->root.rotation[i][k] * local->rotation[k][j];
                world->rotation[i][j] = (int16_t)(turn >> 12);
            }
        }
        world->translation[i] += machine->root.translation[i];
    }
}

int bfm_steam_knight_machine_gauntlet_socket(const BfmSteamKnightMachine *machine,
                                           BfmForestTransform *world)
{
    if (!machine || !machine->active || !world) return 0;
    unsigned tick = machine->controlled ? machine->control_age % 32 : entrance_clock(machine->entrance_age);
    const BfmForestTransform *waist = &machine->chassis_pose->local[21];
    const BfmForestTransform *reference = &sample(&machine->chassis[NEUTRAL], tick)->local[21];
    const BfmForestTransform *part = &sample(&machine->torso, tick)->local[GAUNTLET];
    BfmForestTransform mounted;
    mounted_upper_part(waist, reference, part, &mounted);
    world_part(machine, &mounted, world);
    return 1;
}

int bfm_steam_knight_machine_joint(const BfmSteamKnightMachine *machine, unsigned chassis_part,
                                  BfmForestTransform *world, const MusashiForestPlayerPart **mesh_part)
{
    if (!machine || !machine->active || chassis_part >= model[CHASSIS].parts || !world || !mesh_part)
        return 0;
    const BfmForestTransform *local = &machine->chassis_pose->local[chassis_part];
    world_part(machine, local, world);
    *mesh_part = musashi_forest_player_mesh_part(machine->mesh[CHASSIS], chassis_part);
    return *mesh_part != NULL;
}

int bfm_steam_knight_machine_root_q12(const BfmSteamKnightMachine *machine, int32_t position[3])
{
    if (!machine || !machine->active || !position) return 0;
    position[0] = machine->x4 * 1024 + machine->root_remainder[0];
    position[1] = machine->root.translation[1] * 4096 + (int)machine->y_fraction * 512 + machine->root_remainder[1];
    position[2] = machine->z4 * 1024 + machine->root_remainder[2];
    return 1;
}

int bfm_steam_knight_machine_round_pose(BfmSteamKnightMachine *machine, unsigned stage,
                                       unsigned round, unsigned active_age)
{
    if (!machine || !machine->controlled || round > 2 ||
        (stage != 8 && stage != 9 && stage != 10)) return 0;
    if (machine->round_stage != stage || machine->action != BFM_STEAM_MACHINE_TRANSFERRING) {
        select_clip(machine, stage == 10 ? COLLAPSE : stage == 8 ? DEPARTURE : ARRIVAL, 0);
        machine->action = BFM_STEAM_MACHINE_TRANSFERRING;
        machine->round_stage = stage;
        machine->walk_after_strike = machine->patrol_after_recovery = 0;
    }
    machine->round = round;
    machine->phase_age = active_age;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    return 1;
}

int bfm_steam_knight_machine_reset_round(BfmSteamKnightMachine *machine, unsigned round)
{
    if (!machine || !machine->controlled || round > 2) return 0;
    select_clip(machine, NEUTRAL, 0);
    machine->action = BFM_STEAM_MACHINE_NEUTRAL;
    machine->round = round;
    machine->round_stage = machine->opening_attacks = 0;
    machine->knee_seen = round != 0;
    machine->new_break_in_strike = machine->walk_after_strike = machine->patrol_after_recovery = 0;
    machine->neutral_since = machine->control_age;
    /* The first transfer rearms both legs. The street retains the damage. */
    if (round != 2) machine->broken_shells = 0;
    if (!round) {
        int16_t at[3];
        bfm_steam_knight_machine_world_root(machine, at);
        bfm_steam_knight_machine_set_world_root(machine, at);
        machine->open_cover = machine->yaw = 0;
        bfm_forest_actor_root(0, at, &machine->root);
        machine->display_root = machine->root;
    }
    machine->shell_white_from = machine->shell_white_until = 0;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    return 1;
}

void bfm_steam_knight_machine_open_cover(BfmSteamKnightMachine *machine)
{
    if (machine && machine->controlled) machine->open_cover = 1;
}

int bfm_steam_knight_machine_weak_tip(const BfmForestTransform *child, int16_t world[3])
{
    if (!child || !world) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        int local = 22 * child->rotation[i][1] - 108 * child->rotation[i][2];
        int64_t point = (int64_t)child->translation[i] + (local >> 12);
        if (point < INT16_MIN || point > INT16_MAX) return 0;
        world[i] = (int16_t)point;
    }
    return 1;
}

size_t bfm_steam_knight_machine_weak_halo(const BfmForestTransform *child,
    const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity)
{
    if (!camera || !cards || capacity < BFM_STEAM_WEAK_HALO_CARDS) return 0;
    int16_t points[4][3];
    if (!bfm_steam_knight_machine_weak_tip(child, points[0])) return 0;
    for (unsigned k = 1; k < 4; ++k)
        for (unsigned i = 0; i < 3; ++i) points[k][i] = points[0][i];
    BfmForestDustCard projected = {0};
    if (!bfm_forest_project_effect_quad(camera, points, &projected) || !projected.bucket ||
        projected.bucket >= 4096) return 0;
    /* Two visible original openings give radii of 42.35 and 42.52 world
     * units. Approximate their additive, linearly fading green light. */
    double radius = 42.5 * camera->projection / (4 * projected.bucket);
    int x = projected.vertices[0].x, y = projected.vertices[0].y;
    BfmForestRasterVertex edge[BFM_STEAM_WEAK_HALO_CARDS];
    for (unsigned i = 0; i < BFM_STEAM_WEAK_HALO_CARDS; ++i) {
        double angle = 6.283185307179586 * i / BFM_STEAM_WEAK_HALO_CARDS;
        edge[i] = (BfmForestRasterVertex){
            .x = x + (int)lround(radius * cos(angle)), .y = y + (int)lround(radius * sin(angle))
        };
    }
    for (unsigned i = 0; i < BFM_STEAM_WEAK_HALO_CARDS; ++i) {
        cards[i] = (BfmForestDustCard){.slot = i, .bucket = projected.bucket,
            .material = {.page = 1u << 5, .count = 3, .translucent = 1},
            .vertices = {{.x = x, .y = y, .rgb = UINT32_C(0x00e038)}, edge[i],
                         edge[(i + 1) % BFM_STEAM_WEAK_HALO_CARDS]}};
    }
    return BFM_STEAM_WEAK_HALO_CARDS;
}

typedef struct {
    unsigned clip, key, extension;
    int y8, at_destination, turn, landed;
} StreetSample;

/* A grounded windup, a fixed-velocity rise and a delayed accelerating fall.
 * The encounter supplies the actual landing release and whether its cap opens. */
static int street_sample(const BfmSteamKnightJump *jump, unsigned age, StreetSample *out)
{
    unsigned apex = jump->rebound ? 33 : 28;
    unsigned lands = apex + 62;
    *out = (StreetSample){.clip = JUMP_START, .at_destination = age >= apex,
        .turn = age >= apex + 17, .landed = age == lands};
    if (age < 3) {
        out->key = age ? age - 1 : 0;
        return 1;
    }
    if (age < lands) {
        out->clip = AIRBORNE;
        out->key = age - 3 < 13 ? age - 3 : 13;
        if (age > 14) {
            unsigned rise = (age < apex ? age : apex) - 14;
            out->y8 = -448 * (int)rise;
            if (jump->rebound) out->y8 += (int)(6 * rise * (rise + 1));
            if (age > apex + 16) {
                unsigned fall = age - apex - 16;
                out->y8 += (int)(3 * fall * (fall + 1) + jump->fraction_eighths);
            }
        }
        return 1;
    }
    unsigned grounded = age - lands;
    out->clip = LANDED;
    out->key = grounded < 6 ? grounded : 6;
    if (jump->exposed && grounded > 6)
        out->extension = grounded < 12 ? 4 * (grounded - 6) : 24;
    if (jump->release_after_landing >= 7 && grounded >= (unsigned)jump->release_after_landing) {
        unsigned recover = grounded - (unsigned)jump->release_after_landing;
        if (recover >= 8) return 0;
        out->key = 7 + recover;
        if (jump->exposed) out->extension = recover < 5 ? 20 - 4 * recover : 0;
    }
    return 1;
}

int bfm_steam_knight_machine_street_jump(BfmSteamKnightMachine *machine,
    const BfmSteamKnightJump *jump, unsigned active_age, BfmSteamKnightLanding *landing)
{
    if (!machine || !jump || !landing || !machine->controlled || machine->round != 2 ||
        jump->yaw_before >= 4096 || jump->yaw_after >= 4096 || jump->fraction_eighths > 7)
        return 0;
    StreetSample now;
    if (!street_sample(jump, active_age, &now)) return 0;
    int y = jump->start[1] + (now.y8 >> 3);
    if (y < INT16_MIN || y > INT16_MAX) return 0;

    if (machine->action != BFM_STEAM_MACHINE_JUMPING || active_age < machine->phase_age) {
        machine->previous_jump_pose = machine->chassis_pose;
        machine->outgoing[0] = machine->display[CHASSIS];
        machine->outgoing[1] = machine->chassis_pose;
        machine->action = BFM_STEAM_MACHINE_JUMPING;
        machine->walk_after_strike = machine->patrol_after_recovery = 0;
    }
    machine->phase_age = active_age;
    machine->jump_now = active_age ? &machine->chassis[now.clip].pose[now.key] :
                                     machine->previous_jump_pose;
    if (active_age < 2) machine->jump_shown = machine->outgoing[active_age];
    else if (active_age == 2) machine->jump_shown = machine->previous_jump_pose;
    else {
        StreetSample shown;
        street_sample(jump, active_age - 2, &shown);
        machine->jump_shown = &machine->chassis[shown.clip].pose[shown.key];
    }
    int16_t at[3] = {now.at_destination ? jump->destination[0] : jump->start[0],
                     (int16_t)y, now.at_destination ? jump->destination[1] : jump->start[2]};
    bfm_steam_knight_machine_set_world_root(machine, at);
    machine->y_fraction = (unsigned)now.y8 & 7;
    machine->yaw = now.turn ? jump->yaw_after : jump->yaw_before;
    bfm_forest_actor_root(machine->yaw, at, &machine->root);
    machine->display_root = machine->root;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    landing->core_extension = now.extension;
    landing->touched_ground = now.landed;
    return 1;
}

static int street_position(BfmSteamKnightMachine *machine, const int32_t position[3], unsigned yaw)
{
    if (!machine || !machine->controlled || machine->round != 2 || !position || yaw >= 4096)
        return 0;
    int16_t at[3];
    for (unsigned i = 0; i < 3; ++i) {
        int32_t integer = position[i] >> 12;
        if (integer < INT16_MIN || integer > INT16_MAX) return 0;
        at[i] = (int16_t)integer;
    }
    bfm_steam_knight_machine_set_world_root(machine, at);
    machine->x4 = position[0] >> 10;
    machine->z4 = position[2] >> 10;
    machine->y_fraction = ((unsigned)position[1] >> 9) & 7;
    machine->root_remainder[0] = (unsigned)position[0] & 1023;
    machine->root_remainder[1] = (unsigned)position[1] & 511;
    machine->root_remainder[2] = (unsigned)position[2] & 1023;
    machine->yaw = yaw;
    bfm_forest_actor_root(yaw, at, &machine->root);
    machine->display_root = machine->root;
    return 1;
}

int bfm_steam_knight_machine_street_walk(BfmSteamKnightMachine *machine, unsigned active_age,
                                        const int32_t position[3], unsigned yaw)
{
    if (!street_position(machine, position, yaw)) return 0;
    if (machine->action != BFM_STEAM_MACHINE_STREET_WALKING || active_age < machine->phase_age) {
        select_clip(machine, STEP_FORWARD, 0);
        machine->action = BFM_STEAM_MACHINE_STREET_WALKING;
        machine->walk_after_strike = machine->patrol_after_recovery = 0;
    }
    machine->phase_age = active_age;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    return 1;
}

int bfm_steam_knight_machine_street_shot(BfmSteamKnightMachine *machine, int marker_selected,
    unsigned active_age, int recovery_age, const int32_t position[3], unsigned yaw)
{
    unsigned windup = marker_selected ? 15 : 31;
    if (recovery_age < -1 || recovery_age >= 16) return 0;
    if (recovery_age >= 0 && ((unsigned)recovery_age > active_age ||
        active_age - (unsigned)recovery_age < windup + 8)) return 0;
    if (!street_position(machine, position, yaw)) return 0;
    if (machine->action != BFM_STEAM_MACHINE_STREET_SHOT || active_age < machine->phase_age ||
        machine->shot_windup != windup) {
        select_clip(machine, FIRST_ACTION, 0);
        machine->action = BFM_STEAM_MACHINE_STREET_SHOT;
        machine->walk_after_strike = machine->patrol_after_recovery = 0;
    }
    machine->phase_age = active_age;
    machine->shot_windup = windup;
    machine->shot_recovering = recovery_age >= 0;
    if (machine->shot_recovering) machine->shot_release = active_age - (unsigned)recovery_age;
    bfm_steam_knight_machine_present(machine, machine->control_age);
    return 1;
}

int bfm_steam_knight_machine_street_stand(BfmSteamKnightMachine *machine)
{
    if (!machine || !machine->controlled || machine->round != 2) return 0;
    if (machine->action != BFM_STEAM_MACHINE_NEUTRAL) {
        select_clip(machine, NEUTRAL, 0);
        machine->action = BFM_STEAM_MACHINE_NEUTRAL;
        machine->neutral_since = machine->control_age;
        machine->walk_after_strike = machine->patrol_after_recovery = 0;
        bfm_steam_knight_machine_present(machine, machine->control_age);
    }
    return 1;
}

int bfm_steam_knight_machine_knee_pose(const BfmSteamKnightMachine *machine,
                                      unsigned *key, unsigned *hold_age, int *recovering)
{
    if (!machine || !key || !hold_age || !recovering ||
        (machine->action != BFM_STEAM_MACHINE_KNEELING &&
         machine->action != BFM_STEAM_MACHINE_GETTING_UP)) return 0;
    unsigned age = machine->control_age - machine->clip_epoch;
    unsigned last = machine->selected->count - 1;
    *recovering = machine->action == BFM_STEAM_MACHINE_GETTING_UP;
    *key = *recovering ? (age < last ? last - age : 0) : (age / 2 < last ? age / 2 : last);
    *hold_age = !*recovering && age >= last * 2 ? age - last * 2 : 0;
    return 1;
}

int bfm_steam_knight_machine_core_socket(const BfmSteamKnightMachine *machine,
                                         int waist, BfmForestTransform *world)
{
    const MusashiForestPlayerPart *unused = NULL;
    if (!bfm_steam_knight_machine_joint(machine, waist ? 21 : 0, world, &unused)) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        int offset = 16 * world->rotation[i][1];
        if (waist) offset += 24 * world->rotation[i][2];
        world->translation[i] += offset >> 12;
    }
    return 1;
}

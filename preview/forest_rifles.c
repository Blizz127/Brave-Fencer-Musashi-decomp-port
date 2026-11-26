/* SPDX-License-Identifier: MIT */
#include "forest_rifles.h"
#include "forest_shore_motion.h"
#include "forest_guard_clip.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum RifleAction { ABSENT, FIRE, LOWER, IDLE, RECOIL };

typedef struct {
    int16_t at[3], placement_yaw, yaw, muzzle[3];
    uint16_t health, last_attack;
    int recoil_pending;
    BfmForestShoreMotion recoil;
    enum RifleAction action;
    unsigned key, cycles, light;
    BfmForestPose pose;
    BfmForestTransform root, flash_root;
    BfmForestActor actor, flash;
} ForestRifle;

struct BfmForestRifles {
    ForestRifle placed[BFM_FOREST_BANK_RIFLES];
    MusashiForestPlayerMesh *gunner, *flash;
    BfmForestPose rigid;
    BfmForestGuardClip clip[4];
    uint16_t starting_health;
    const BfmForestField *field;
    int16_t muzzle_offset[3];
};

static int floor_q12(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

MusashiForestPlayerMesh *bfm_forest_rifle_mesh(const BfmForestField *field, uint32_t table,
                                               unsigned wanted)
{
    if (!wanted || wanted > MUSASHI_FOREST_PLAYER_PARTS) return NULL;
    size_t length = (wanted + 1u) * 4;
    const uint8_t *parts = bfm_forest_room_bytes(field, table, length);
    if (!parts || u32le(parts + wanted * 4)) return NULL;
    uint32_t start = u32le(parts);
    if (start >= table || table - start > (1u << 20)) return NULL;
    length += table - start;
    const uint8_t *scene = bfm_forest_room_bytes(field, start, length);
    unsigned count;
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_from_table(scene, length,
                                                                         start, table, &count);
    if (mesh && count == wanted) return mesh;
    musashi_forest_player_mesh_free(mesh);
    return NULL;
}

BfmForestRifles *bfm_forest_rifles_open(const BfmForestField *field)
{
    if (!field) return NULL;
    const uint8_t *room = bfm_forest_room_bytes(field, UINT32_C(0x80186ad0), 4);
    const uint8_t *desc = room ? bfm_forest_room_bytes(field, u32le(room), 0xc4) : NULL;
    if (!desc || u32le(desc + 0xbc) != UINT32_C(0x8013e298) ||
        u32le(desc + 0xc0) != UINT32_C(0x8012bf7c)) return NULL;
    const uint8_t *list = bfm_forest_room_bytes(field, u32le(desc + 0xb8), 11 * 20);
    const uint8_t *offset = bfm_forest_room_bytes(field, UINT32_C(0x8018aa6c), 8);
    const uint8_t *gun = bfm_forest_room_bytes(field, UINT32_C(0x8018abc0) + 5 * 52, 52);
    const uint8_t *light = bfm_forest_room_bytes(field, UINT32_C(0x8019918c), 8);
    if (!list || !offset || !gun || !light || s16le(gun) <= 0) return NULL;
    BfmForestRifles *rifles = calloc(1, sizeof *rifles);
    if (!rifles) return NULL;
    rifles->field = field;
    for (unsigned i = 0; i < 3; ++i) rifles->muzzle_offset[i] = s16le(offset + 2 * i);
    rifles->starting_health = u16le(gun);
    rifles->gunner = bfm_forest_rifle_mesh(field, u32le(gun + 4), BFM_GUARD_PARTS);
    rifles->flash = bfm_forest_rifle_mesh(field, u32le(light + 4), 1);
    if (!rifles->gunner || !rifles->flash ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b0d50), 0xffff, &rifles->clip[FIRE - 1]) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b0e48), 0xffff, &rifles->clip[LOWER - 1]) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801afdd0), 0xffff, &rifles->clip[IDLE - 1]) ||
        !bfm_forest_guard_clip_read(field, UINT32_C(0x801b02a8), 0xffff, &rifles->clip[RECOIL - 1]) ||
        rifles->clip[RECOIL - 1].count != 16 || rifles->clip[FIRE - 1].count != 30 || rifles->clip[LOWER - 1].count != 30 ||
        rifles->clip[IDLE - 1].count != 40) goto failed;
    for (unsigned i = 0; i < 3; ++i) rifles->rigid.local[0].rotation[i][i] = 4096;
    for (unsigned i = 0; i < BFM_FOREST_BANK_RIFLES; ++i) {
        const uint8_t *p = list + 20 * (4 + 3 * i);
        ForestRifle *r = &rifles->placed[i];
        if (u16le(p + 6) != 47 || u16le(p + 8) != 0x500 + i) goto failed;
        for (unsigned axis = 0; axis < 3; ++axis) r->at[axis] = s16le(p + 2 * axis);
        r->placement_yaw = s16le(p + 12);
        r->actor = (BfmForestActor){.mesh = rifles->gunner, .root = &r->root, .pose = &r->pose,
                                  .visible_parts = (1u << BFM_GUARD_PARTS) - 1};
        r->flash = (BfmForestActor){.mesh = rifles->flash, .root = &r->flash_root, .pose = &rifles->rigid, .visible_parts = 1};
    }
    return rifles;
failed:
    bfm_forest_rifles_free(rifles);
    return NULL;
}

void bfm_forest_rifles_free(BfmForestRifles *rifles)
{
    if (!rifles) return;
    musashi_forest_player_mesh_free(rifles->gunner);
    musashi_forest_player_mesh_free(rifles->flash);
    free(rifles);
}

static int select_pose(ForestRifle *rifle, const BfmForestRifles *rifles)
{
    const BfmForestGuardClip *clip = &rifles->clip[rifle->action - 1];
    return bfm_forest_actor_pose_decode(clip->pose[rifle->key], BFM_GUARD_POSE_BYTES,
                                        BFM_GUARD_PARTS, &rifle->pose);
}

static void aim_rifle(const BfmForestRifles *rifles, ForestRifle *rifle, const int16_t at[3])
{
    bfm_forest_actor_root((unsigned)rifle->yaw, at, &rifle->root);
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k)
            dot += (int64_t)rifle->root.rotation[axis][k] * rifles->muzzle_offset[k];
        rifle->muzzle[axis] = (int16_t)(at[axis] + floor_q12(dot));
    }
}

int bfm_forest_rifles_place(BfmForestRifles *rifles, int player_z)
{
    if (!rifles) return 0;
    for (unsigned i = 0; i < BFM_FOREST_BANK_RIFLES; ++i) {
        ForestRifle *r = &rifles->placed[i];
        int64_t dz = llabs((int64_t)r->at[2] - player_z);
        if (r->action == ABSENT && dz >= 705 && dz < 768) {
            r->action = FIRE;
            r->health = rifles->starting_health;
            r->last_attack = 0;
            r->key = r->cycles = r->light = 0;
            r->recoil_pending = 0;
            r->yaw = r->placement_yaw;
            aim_rifle(rifles, r, r->at);
            if (!select_pose(r, rifles)) return 0;
        }
    }
    return 1;
}

/* Orient the mounted flash around the rifle bore in the rifle frame. */
static void light_muzzle(ForestRifle *r, unsigned roll)
{
    BfmForestTransform turn;
    bfm_forest_actor_root(roll & 4095u, r->muzzle, &turn);
    int c = turn.rotation[0][0], s = turn.rotation[0][2];
    const int local[3][3] = {{floor_q12(6144 * c), floor_q12(-6144 * s), 0},
                           {floor_q12(6144 * s), floor_q12(6144 * c), 0}, {0, 0, 6144}};
    for (unsigned i = 0; i < 3; ++i) {
        r->flash_root.translation[i] = r->muzzle[i];
        for (unsigned j = 0; j < 3; ++j) {
            int64_t dot = 0;
            for (unsigned k = 0; k < 3; ++k) dot += (int64_t)r->root.rotation[i][k] * local[k][j];
            r->flash_root.rotation[i][j] = (int16_t)floor_q12(dot);
        }
    }
    r->light = 128;
}

static void start_recoil(ForestRifle *r, uint16_t attacker_yaw)
{
    r->action = RECOIL;
    r->key = 0;
    r->recoil_pending = 1;
    r->yaw = (int16_t)((attacker_yaw + 2048u) & 4095u);
    for (unsigned axis = 0; axis < 3; ++axis)
        r->recoil.position[axis] = (int16_t)r->root.translation[axis];
    r->recoil.y_q16 = (int32_t)r->recoil.position[1] * 65536;
    r->recoil.recoil_vy_q16 = 0;
}

int bfm_forest_rifle_struck(BfmForestRifles *rifles, unsigned index, uint16_t attacker_yaw)
{
    if (!rifles || index >= BFM_FOREST_BANK_RIFLES) return 0;
    ForestRifle *r = &rifles->placed[index];
    if (r->action == ABSENT || r->action == RECOIL) return 0;
    start_recoil(r, attacker_yaw);
    return 1;
}

int bfm_forest_rifle_take_hit(BfmForestRifles *rifles, unsigned index,
                              uint16_t attacker_yaw, uint16_t strength, uint16_t attack_type)
{
    if (!rifles || index >= BFM_FOREST_BANK_RIFLES) return 0;
    ForestRifle *rifle = &rifles->placed[index];
    if (rifle->action == ABSENT || !attack_type || attack_type == rifle->last_attack ||
        !strength || strength >= rifle->health) return 0;
    rifle->last_attack = attack_type;
    start_recoil(rifle, attacker_yaw);
    rifle->health -= strength;
    return 1;
}

static int recoil_tick(BfmForestRifles *rifles, ForestRifle *r, BfmGameplayRun *run)
{
    int16_t prior_y = r->recoil.position[1];
    if (!bfm_forest_shore_recoil_height(&r->recoil, rifles->field)) return 0;
    if (r->recoil_pending) {
        r->recoil_pending = 0;
        aim_rifle(rifles, r, r->recoil.position);
        r->root.translation[1] = prior_y;
    } else {
        aim_rifle(rifles, r, r->recoil.position);
        if (++r->key + 1 == rifles->clip[RECOIL - 1].count) {
            /* All guards select a patrol delay here. A rifleman immediately
             * replaces the patrol with a fresh firing sequence. */
            bfm_gameplay_random(run);
            r->action = FIRE;
            r->last_attack = 0;
            r->cycles = r->key = 0;
        }
    }
    return 1;
}

int bfm_forest_rifle_tick(BfmForestRifles *rifles, unsigned index, int player_z,
                          BfmGameplayRun *run, BfmForestRifleShot *shot)
{
    if (!rifles || index >= BFM_FOREST_BANK_RIFLES || !run) return 0;
    ForestRifle *r = &rifles->placed[index];
    r->light = r->light == 128 ? 64 : 0;
    if (r->action == ABSENT) return 1;
    int fired = 0;
    if (r->action == RECOIL) {
        if (!recoil_tick(rifles, r, run)) return 0;
    } else if (++r->key == rifles->clip[r->action - 1].count) {
        r->key = 0;
        ++r->cycles;
        if (r->action == FIRE && r->cycles == 3) {
            r->action = LOWER;
            r->cycles = 0;
        } else if (r->action == LOWER) {
            r->action = IDLE;
            r->cycles = 0;
        } else if (r->action == IDLE && (bfm_gameplay_random(run) & 1u) < r->cycles) {
            r->action = FIRE;
            r->cycles = 0;
        }
    } else if (r->action == FIRE && r->key == 16) {
        light_muzzle(r, bfm_gameplay_random(run));
    } else if (r->action == FIRE && r->key == 19) {
        if (shot) {
            for (unsigned i = 0; i < 3; ++i) shot->muzzle[i] = r->muzzle[i];
            shot->yaw = r->yaw;
        }
        fired = 1;
    }
    if (llabs((int64_t)player_z - r->at[2]) >= 864) r->action = ABSENT;
    else if (!select_pose(r, rifles)) return 0;
    return fired ? 2 : 1;
}

const BfmForestActor *bfm_forest_rifle_actor(const BfmForestRifles *rifles, unsigned index)
{
    if (!rifles || index >= BFM_FOREST_BANK_RIFLES || rifles->placed[index].action == ABSENT) return NULL;
    return &rifles->placed[index].actor;
}

const BfmForestActor *bfm_forest_rifle_flash(const BfmForestRifles *rifles,
                                             unsigned index, unsigned *luma)
{
    if (!rifles || index >= BFM_FOREST_BANK_RIFLES || !rifles->placed[index].light) return NULL;
    if (luma) *luma = rifles->placed[index].light;
    return &rifles->placed[index].flash;
}

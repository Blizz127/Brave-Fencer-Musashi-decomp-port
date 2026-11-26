/* SPDX-License-Identifier: MIT */
#include "forest_fall.h"
#include "forest_opening_data.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_forest_fall_load(BfmForestFall *fall, const BfmForestField *field, uint32_t placements)
{
    const uint8_t *actors = bfm_forest_room_bytes(field, placements, 60);
    const uint8_t *sprites = bfm_forest_room_bytes(field, UINT32_C(0x8018a7c0), 4 * 12);
    if (!fall || !actors || u16le(actors + 46) != 34 || u16le(actors + 48) != 0 || !sprites)
        return 0;
    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *cell = sprites + i * 12;
        if (!cell[2] || !cell[3]) return 0;
    }
    *fall = (BfmForestFall){.sprites = sprites, .timer = 1};
    return 1;
}

void bfm_forest_fall_set_other_slots(BfmForestFall *fall, unsigned effects, unsigned drawables)
{
    fall->other_effects = effects;
    fall->other_drawables = drawables;
}

static int16_t signed_word(int64_t value)
{
    unsigned word = (uint16_t)value;
    return (int16_t)((int)(word ^ 32768u) - 32768);
}

static int16_t world_units(int64_t fixed)
{
    return signed_word(fixed / 65536 - (fixed % 65536 < 0));
}

void bfm_forest_fall_emit(BfmForestFall *fall, BfmGameplayRun *run, const int16_t player[3],
                          unsigned selector, unsigned side)
{
    /* Other forest effects share 128 effect records and 130 drawables. */
    if (!fall || !fall->sprites || !run || !player || fall->count == BFM_FOREST_FALL_SLOTS ||
        fall->other_effects >= BFM_FOREST_FALL_SLOTS - fall->count ||
        fall->other_drawables >= 130 - fall->count) return;
    BfmForestFallingParticle *particle = fall->particles;
    while (particle->sprite.kind) ++particle;
    int x = (side & 1 ? 122 : -248) + (bfm_gameplay_random(run) & 255);
    int y = signed_word(player[1] - (side & 1 ? 224 : 192));
    int z = signed_word(player[2] - 320 + bfm_gameplay_random(run) % 640);
    *particle = (BfmForestFallingParticle){
        .sprite = {.position = {(int16_t)x, (int16_t)y, (int16_t)z}, .kind = 1 + (selector & 1)},
        .position = {x * INT64_C(65536), y * INT64_C(65536), z * INT64_C(65536)},
        .floor = signed_word(player[1] - 80)
    };
    ++fall->count;
}

void bfm_forest_fall_tick_emitter(BfmForestFall *fall, BfmGameplayRun *run,
                                  const int16_t player[3])
{
    if (!fall || !run || !player || !fall->timer) return;
    if (fall->timer % 16 == 0) {
        unsigned kind = bfm_gameplay_random(run);
        bfm_forest_fall_emit(fall, run, player, kind, 0);
    }
    if (--fall->timer == 0) {
        fall->timer = 48;
        unsigned kind = bfm_gameplay_random(run);
        unsigned side = bfm_gameplay_random(run);
        bfm_forest_fall_emit(fall, run, player, kind, side);
    }
}

static void start_particle(BfmForestFallingParticle *p, BfmGameplayRun *run)
{
    p->sprite.cell = bfm_gameplay_random(run) & 3;
    if (p->sprite.kind == 1) {
        const int16_t origin[3] = {0};
        BfmForestTransform turn;
        bfm_forest_actor_root(bfm_gameplay_random(run) & 4095u, origin, &turn);
        p->acceleration_x = turn.rotation[0][0] * 4;
        p->acceleration_z = turn.rotation[2][0] * 4;
        p->velocity[1] = 65536;
    } else {
        p->velocity[1] = 49152 + bfm_gameplay_random(run);
        p->velocity[0] = 2 * (bfm_gameplay_random(run) & 16383) - 16384;
        p->velocity[2] = 2 * (bfm_gameplay_random(run) & 16383) - 16384;
    }
    p->ready = 1;
}

static void advance_particle(BfmForestFallingParticle *p)
{
    for (unsigned i = 0; i < 3; ++i) p->position[i] += p->velocity[i];
    if (p->sprite.kind == 1) {
        p->position[1] += 65536;
        p->velocity[0] += p->acceleration_x;
        p->velocity[2] += p->acceleration_z;
        p->phase = (p->phase + 1) % 16;
        if (p->phase == 8) {
            p->acceleration_x = -p->acceleration_x;
            p->acceleration_z = -p->acceleration_z;
        }
        if (p->phase % 8 == 0) p->velocity[1] = -p->velocity[1];
    } else {
        p->sprite.angle = (uint16_t)(p->sprite.angle + 512);
    }
    for (unsigned i = 0; i < 3; ++i) p->sprite.position[i] = world_units(p->position[i]);
}

void bfm_forest_fall_step(BfmForestFall *fall, BfmGameplayRun *run)
{
    if (!fall || !run) return;
    for (unsigned i = 0; i < BFM_FOREST_FALL_SLOTS; ++i) {
        BfmForestFallingParticle *p = fall->particles + i;
        if (!p->sprite.kind) continue;
        if (!p->ready) {
            start_particle(p, run);
            continue;
        }
        advance_particle(p);
        if (p->sprite.position[1] > p->floor) {
            p->sprite.kind = 0;
            --fall->count;
        }
    }
}

unsigned bfm_forest_fall_project(const BfmForestFall *fall, const BfmForestCamera *camera,
                                 BfmForestDustCard *out, unsigned capacity)
{
    if (!fall || !camera || !out || !fall->sprites) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_FOREST_FALL_SLOTS && count < capacity; ++i) {
        const BfmForestFallingParticle *p = &fall->particles[i];
        if (p->sprite.kind && p->ready && bfm_forest_fall_project_sprite(&p->sprite,
                fall->sprites + 12 * p->sprite.cell, camera, &out[count])) {
            out[count++].slot = i;
        }
    }
    return count;
}

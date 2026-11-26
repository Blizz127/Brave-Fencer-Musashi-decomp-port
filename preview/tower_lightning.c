/* SPDX-License-Identifier: MIT */
#include "tower_lightning.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>

enum { POSTS = 4, SEGMENTS = 16 };

typedef struct {
    int16_t offset[POSTS][SEGMENTS + 1];
    unsigned visible;
} TowerArcShape;

struct BfmTowerCircuit {
    int16_t post[POSTS][3];
    TowerArcShape submitted[3];
    unsigned phase, intact, current;
};

BfmTowerCircuit *bfm_tower_lightning_load(const uint8_t *room, size_t bytes)
{
    const size_t endpoints = UINT32_C(0x8018a414) - UINT32_C(0x80128158);
    if (!room || bytes < endpoints + POSTS * 16) return NULL;
    BfmTowerCircuit *circuit = calloc(1, sizeof *circuit);
    if (!circuit) return NULL;
    for (unsigned i = 0; i < POSTS; ++i) {
        const uint8_t *pair = room + endpoints + 16 * i;
        const uint8_t *next = room + endpoints + 16 * ((i + 1) % POSTS);
        for (unsigned k = 0; k < 3; ++k) {
            int begin = s16le(pair + 2*k), end = s16le(pair + 8 + 2*k);
            int span = end - begin;
            if (end != s16le(next + 2*k) || span < INT16_MIN || span > INT16_MAX ||
                (k == 1 && span)) goto invalid;
            circuit->post[i][k] = (int16_t)begin;
        }
        if (s16le(pair) == s16le(next) && s16le(pair + 4) == s16le(next + 4)) goto invalid;
    }
    circuit->intact = (1u << POSTS) - 1;
    return circuit;
invalid:
    free(circuit);
    return NULL;
}

void bfm_tower_lightning_free(BfmTowerCircuit *circuit)
{
    free(circuit);
}

/* Only the eight equally spaced phases of one sine wave are sampled. */
static int wave(unsigned phase)
{
    unsigned angle = phase & 7u;
    int height = (angle & 1u) ? 2896 : (angle & 2u) ? 4096 : 0;
    return angle < 4 ? height : -height;
}

void bfm_tower_lightning_tick(BfmTowerCircuit *c, BfmGameplayRun *run, unsigned broken_pylons)
{
    if (!c || !run) return;
    c->intact &= ~broken_pylons;
    c->current = (c->current + 1) % 3;
    TowerArcShape *shape = &c->submitted[c->current];
    shape->visible = 0;
    for (unsigned edge = 0; edge < POSTS; ++edge) {
        unsigned pair = (1u << edge) | (1u << ((edge + 1) % POSTS));
        if ((c->intact & pair) != pair) continue;
        shape->visible |= 1u << edge;
        for (unsigned i = 1; i <= SEGMENTS; ++i) {
            int amplitude = 8;
            if (i < SEGMENTS) {
                amplitude += bfm_gameplay_random(run) & 31;
                amplitude += bfm_gameplay_random(run) & 31;
            }
            shape->offset[edge][i] = (int16_t)(amplitude * wave(c->phase + i) / 4096);
        }
    }
    c->phase = (c->phase + 1) & 7u;
}

size_t bfm_tower_lightning_cards(const BfmTowerCircuit *c, unsigned delay, const BfmForestCamera *camera,
                                 BfmForestDustCard *out, size_t capacity)
{
    if (!c || delay > 2 || !camera || !out || !capacity) return 0;
    const TowerArcShape *shape = &c->submitted[(c->current + 3 - delay) % 3];
    size_t count = 0;
    for (unsigned edge = 0; edge < POSTS && count < capacity; ++edge) {
        if (!(shape->visible & (1u << edge))) continue;
        const int16_t *start = c->post[edge], *end = c->post[(edge + 1) % POSTS];
        BfmForestTransform root = {.rotation={{4096,0,0},{0,4096,0},{0,0,4096}},
                                   .translation={start[0], start[1], start[2]}};
        BfmForestCamera local;
        if (!bfm_forest_effect_camera(camera, &root, &local)) continue;
        int dx = end[0] - start[0], dz = end[2] - start[2];
        int16_t x = 0, z = 0;
        for (unsigned i = 1; i <= SEGMENTS && count < capacity; ++i) {
            int16_t next_x = (int16_t)(dx * (int)i / SEGMENTS);
            int16_t next_z = (int16_t)(dz * (int)i / SEGMENTS);
            int16_t y = shape->offset[edge][i-1], next_y = shape->offset[edge][i];
            for (int side = -10; side <= 10 && count < capacity; side += 20) {
                int16_t points[4][3] = {
                    {x, y, z}, {next_x, next_y, next_z},
                    {x, (int16_t)(y+side), z}, {next_x, (int16_t)(next_y+side), next_z}
                };
                BfmForestDustCard card = {.material={.count=4, .translucent=1, .page=32}};
                card.vertices[0].rgb = card.vertices[1].rgb = UINT32_C(0xc0e060);
                if (bfm_forest_project_effect_quad(&local, points, &card)) out[count++] = card;
            }
            x = next_x;
            z = next_z;
        }
    }
    return count;
}

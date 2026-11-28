/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_creatures.h"
#include "village_mine_creature_dust.h"

#include <stdlib.h>

enum { FIXED_SLOT = 3, DISTANCE_ROWS = 2 };

struct BfmMineLakeCreatures {
    BfmVillageMineCorridor387 *actors;
    BfmMineCreatureDust *dust;
    BfmMineCreatureShadow shadow[BFM_FOREST_ACTOR_LIMIT];
    uint8_t row_plus_one[BFM_FOREST_ACTOR_LIMIT];
};

void bfm_mine_lake_creatures_free(BfmMineLakeCreatures *room)
{
    if (!room) return;
    bfm_village_mine_corridor_387_free(room->actors);
    bfm_mine_creature_dust_free(room->dust);
    free(room);
}

BfmMineLakeCreatures *bfm_mine_lake_creatures_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const BfmVillageGround *ground, const int16_t pad[3])
{
    if (!disc || !pad) return NULL;
    BfmMineLakeCreatures *room = calloc(1, sizeof *room);
    if (!room) return NULL;
    room->actors = bfm_village_mine_lake_387_open(scene, bytes, ground);
    room->dust = bfm_mine_creature_dust_open(disc);
    if (!room->actors || !room->dust) {
        bfm_mine_lake_creatures_free(room);
        return NULL;
    }
    bfm_village_mine_corridor_387_enter(room->actors, pad, 0);
    return room;
}

static int parent_at(const BfmMineLakeCreatures *room, unsigned slot)
{
    if (slot == FIXED_SLOT && bfm_village_mine_corridor_387_active(room->actors))
        return BFM_MINE_WEST_CREATURE;
    return (int)room->row_plus_one[slot] - 1;
}

int bfm_mine_lake_creatures_occupied(const BfmMineLakeCreatures *room, unsigned slot)
{
    return room && slot < BFM_FOREST_ACTOR_LIMIT && (parent_at(room, slot) >= 0 ||
           room->shadow[slot].active || bfm_mine_creature_dust_occupied(room->dust, slot));
}

static unsigned open_slot(const BfmMineLakeCreatures *room, unsigned first,
    BfmMineLakeSlotInUse occupied, void *context)
{
    for (unsigned slot = first; slot < BFM_FOREST_ACTOR_LIMIT; ++slot)
        if (!occupied(context, slot) && !bfm_mine_lake_creatures_occupied(room, slot)) return slot;
    return BFM_FOREST_ACTOR_LIMIT;
}

void bfm_mine_lake_creatures_admit(BfmMineLakeCreatures *room, const int16_t foot[3],
    BfmMineLakeSlotInUse occupied, void *context)
{
    if (!room || !foot || !occupied) return;
    for (unsigned row = 0; row < DISTANCE_ROWS; ++row) {
        if (!bfm_village_mine_corridor_387_wants_row(room->actors, row, foot)) continue;
        unsigned slot = open_slot(room, 0, occupied, context);
        if (slot == BFM_FOREST_ACTOR_LIMIT) return;
        if (bfm_village_mine_corridor_387_spawn_row(room->actors, row, foot))
            room->row_plus_one[slot] = (uint8_t)(row + 1);
    }
}

int bfm_mine_lake_creatures_visit(BfmMineLakeCreatures *room, unsigned slot,
    const int16_t raw[3], const int16_t published[3], const int16_t target[3],
    BfmGameplayRun *run, unsigned tick, const BfmBedroomView *view,
    BfmMineLakeSlotInUse occupied, void *context, BfmMineLakeCreatureEvent *out)
{
    if (!room || slot >= BFM_FOREST_ACTOR_LIMIT || !raw || !published || !occupied || !out) return 0;
    *out = (BfmMineLakeCreatureEvent){0};
    if (!run) return 1;
    int actor = parent_at(room, slot);
    if (actor < 0) {
        if (room->shadow[slot].active)
            bfm_village_mine_corridor_387_shadow_step(room->actors, &room->shadow[slot]);
        else
            bfm_mine_creature_dust_step_at(room->dust, slot);
        return 1;
    }

    unsigned parent = (unsigned)actor;
    BfmMineCreatureLanding edge;
    out->struck = bfm_village_mine_corridor_387_step_actor(room->actors, parent,
        raw, published, target, run, tick, &out->hit, &edge);
    if (edge.release_shadow) {
        for (unsigned i = slot + 1; i < BFM_FOREST_ACTOR_LIMIT; ++i) {
            if (room->shadow[i].active && room->shadow[i].parent == parent) {
                room->shadow[i].active = 0;
                break;
            }
        }
    }
    if (edge.request_shadow) {
        unsigned child = open_slot(room, slot + 1, occupied, context);
        if (child < BFM_FOREST_ACTOR_LIMIT)
            bfm_village_mine_corridor_387_shadow_start(room->actors, parent, &room->shadow[child]);
    }
    int16_t xyz[3];
    int dust = parent == BFM_MINE_WEST_CREATURE ?
        bfm_village_mine_corridor_387_dust(room->actors, xyz) :
        bfm_village_mine_corridor_387_dust_row(room->actors, parent, xyz);
    if (dust || edge.landed) {
        unsigned child = open_slot(room, slot + 1, occupied, context);
        if (child < BFM_FOREST_ACTOR_LIMIT)
            bfm_mine_creature_dust_spawn_at(room->dust, child, edge.landed ? edge.xyz : xyz);
    }
    if (edge.landed) {
        int pan = bfm_village_mine_corridor_387_landing_pan(&edge, view);
        if (pan >= 0) {
            out->landing_cue = 0x832;
            out->landing_control = 0x2000u | (unsigned)pan;
        }
    }
    if (parent != BFM_MINE_WEST_CREATURE && !bfm_village_mine_corridor_387_has_row(room->actors, parent))
        room->row_plus_one[slot] = 0;
    return 1;
}

int bfm_mine_lake_creatures_draw_slot(const BfmMineLakeCreatures *room, unsigned slot,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!room || slot >= BFM_FOREST_ACTOR_LIMIT || !figure || !view) return 0;
    int parent = parent_at(room, slot);
    if (parent >= 0)
        return bfm_village_mine_corridor_387_draw_body(room->actors, (unsigned)parent, figure, view);
    if (room->shadow[slot].active)
        return bfm_village_mine_corridor_387_shadow_draw(room->actors, &room->shadow[slot], figure, view);
    return !bfm_mine_creature_dust_occupied(room->dust, slot) ||
        bfm_mine_creature_dust_draw_at(room->dust, slot, figure, view);
}

int bfm_mine_lake_creatures_blade(BfmMineLakeCreatures *room, unsigned slot, unsigned event,
    unsigned strength, unsigned yaw, const int16_t from[3], int16_t to[3], int16_t normal[3],
    unsigned *sound)
{
    if (!room || slot >= BFM_FOREST_ACTOR_LIMIT || !sound) return 0;
    int parent = parent_at(room, slot);
    if (parent < 0) return 0;
    int hit = parent == BFM_MINE_WEST_CREATURE ?
        bfm_village_mine_corridor_387_blade(room->actors, event, strength, yaw, from, to, normal) :
        bfm_village_mine_corridor_387_blade_row(room->actors, (unsigned)parent, event, strength,
                                               yaw, from, to, normal);
    if (hit) *sound = bfm_village_mine_corridor_387_blade_sound(room->actors, event);
    return hit;
}

/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_CORRIDOR_387_H
#define BFM_VILLAGE_MINE_CORRIDOR_387_H

#include "bedroom_figure.h"
#include "gameplay_run.h"
#include "village_ground.h"

typedef struct BfmVillageMineCorridor387 BfmVillageMineCorridor387;

typedef struct {
    int16_t origin[3];
    unsigned yaw, flags, power;
} BfmMineLungeHit;

/* The mine encounter borrows its room geometry. It owns the creature mesh. */
BfmVillageMineCorridor387 *bfm_village_mine_corridor_387_open(const uint8_t *scene,
    size_t bytes, const BfmVillageGround *ground);
void bfm_village_mine_corridor_387_free(BfmVillageMineCorridor387 *creature);

/* Dispatch the fixed placement as the room is entered, before the first ordinary field. */
void bfm_village_mine_corridor_387_enter(BfmVillageMineCorridor387 *creature,
    const int16_t hero[3], unsigned world_tick);

/* The physical target and the attacker's tracking root may differ. A NULL
 * tracking root suppresses pursuit. Return one on a new five-power lunge hit. */
int bfm_village_mine_corridor_387_step(BfmVillageMineCorridor387 *creature,
    const int16_t hero[3], const int16_t target[3], BfmGameplayRun *run,
    unsigned world_tick, BfmMineLungeHit *hit);
/* Offer one blade segment after clipping it to nearer surfaces. A contact
 * replaces the endpoint and normal and queues the fixed creature's response. */
int bfm_village_mine_corridor_387_blade(BfmVillageMineCorridor387 *creature,
    unsigned event, unsigned strength, unsigned hero_yaw, const int16_t from[3],
    int16_t to[3], int16_t normal[3]);
/* Dust spawn at the foot of the fixed creature on the last update. */
int bfm_village_mine_corridor_387_dust(const BfmVillageMineCorridor387 *creature, int16_t xyz[3]);
unsigned bfm_village_mine_corridor_387_blade_sound(const BfmVillageMineCorridor387 *creature,
    unsigned event);
int bfm_village_mine_corridor_387_active(const BfmVillageMineCorridor387 *creature);
int bfm_village_mine_corridor_387_draw(const BfmVillageMineCorridor387 *creature,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

enum { BFM_MINE_CEILING_ROWS = 3, BFM_MINE_WEST_CREATURE = BFM_MINE_CEILING_ROWS };
typedef struct {
    int landed;
    int16_t xyz[3];
    int request_shadow, release_shadow;
} BfmMineCreatureLanding;

/* Each granted shadow occupies its own slot after its parent in the field.
 * Its initialization sets the sprite at the parent; its own updates follow the
 * floor until the captured parent disappears or the room releases the slot. */
typedef struct {
    BfmBedroomRoot floor;
    unsigned parent, generation;
    int active, at_birth;
} BfmMineCreatureShadow;
int bfm_village_mine_corridor_387_shadow_start(const BfmVillageMineCorridor387 *creature,
    unsigned parent, BfmMineCreatureShadow *shadow);
int bfm_village_mine_corridor_387_shadow_step(const BfmVillageMineCorridor387 *creature,
    BfmMineCreatureShadow *shadow);
int bfm_village_mine_corridor_387_shadow_draw(const BfmVillageMineCorridor387 *creature,
    const BfmMineCreatureShadow *shadow, BfmBedroomFigure *figure, const BfmBedroomView *view);
int bfm_village_mine_corridor_387_draw_body(const BfmVillageMineCorridor387 *creature,
    unsigned parent, BfmBedroomFigure *figure, const BfmBedroomView *view);
int bfm_village_mine_corridor_387_step_actor(BfmVillageMineCorridor387 *creature, unsigned parent,
    const int16_t raw_hero[3], const int16_t published_foot[3], const int16_t target[3],
    BfmGameplayRun *run, unsigned world_tick, BfmMineLungeHit *hit, BfmMineCreatureLanding *landing);

/* After the occupied actor slots, consider the ceiling rows in placement order.
 * Spawn only in a free slot. Begin stepping that slot on its next field. */
int bfm_village_mine_corridor_387_wants_row(const BfmVillageMineCorridor387 *creature,
    unsigned row, const int16_t reference[3]);
int bfm_village_mine_corridor_387_spawn_row(BfmVillageMineCorridor387 *creature,
    unsigned row, const int16_t reference[3]);
int bfm_village_mine_corridor_387_has_row(const BfmVillageMineCorridor387 *creature, unsigned row);

/* A reserved row keeps its last published hit box while hidden. Test each
 * occupied row in slot order, including hidden rows. Visibility reports the
 * body. Its independent halo may remain visible on the field of a rehang. */
int bfm_village_mine_corridor_387_visible_row(const BfmVillageMineCorridor387 *creature,
    unsigned row);
int bfm_village_mine_corridor_387_blade_row(BfmVillageMineCorridor387 *creature, unsigned row,
    unsigned event, unsigned strength, unsigned hero_yaw, const int16_t from[3],
    int16_t to[3], int16_t normal[3]);
int bfm_village_mine_corridor_387_dust_row(const BfmVillageMineCorridor387 *creature, unsigned row,
    int16_t xyz[3]);

/* A landing requests dust and a screen-panned room sound. The caller checks
 * for retirement after the update before releasing its actor slot. */
int bfm_village_mine_corridor_387_step_row(BfmVillageMineCorridor387 *creature, unsigned row,
    const int16_t hero[3], const int16_t target[3], BfmGameplayRun *run, unsigned world_tick,
    BfmMineLungeHit *hit, BfmMineCreatureLanding *landing);
int bfm_village_mine_corridor_387_landing_pan(const BfmMineCreatureLanding *landing,
    const BfmBedroomView *view);
int bfm_village_mine_corridor_387_draw_row(const BfmVillageMineCorridor387 *creature, unsigned row,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif

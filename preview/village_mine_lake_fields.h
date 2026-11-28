/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_FIELDS_H
#define BFM_VILLAGE_MINE_LAKE_FIELDS_H

#include "bedroom_figure.h"
#include "gameplay_run.h"
#include "village_ground.h"
#include "woods_caster_melee.h"

typedef struct BfmVillageMineLakeFields BfmVillageMineLakeFields;

typedef struct {
    unsigned resident_id, sound;
    int started, reveal, approach;
    const BfmBedroomRoot *root;
    const MusashiForestPlayerMesh *model;
    const uint8_t *resource;
    size_t resource_bytes;
    uint32_t resource_base;
    unsigned parts, pose_tick;
} BfmVillageMineLakeFieldEvent;

/* The mounted SC03/110 scene and live variables must outlive their actors. */
BfmVillageMineLakeFields *bfm_village_mine_lake_fields_open(const uint8_t *scene, size_t bytes,
                                                            const uint8_t variables[256]);
void bfm_village_mine_lake_fields_free(BfmVillageMineLakeFields *fields);
void bfm_village_mine_lake_fields_step(BfmVillageMineLakeFields *fields, BfmGameplayRun *run,
                                      const int16_t player[3], unsigned field_tick);
int bfm_village_mine_lake_fields_body(const BfmVillageMineLakeFields *fields,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne);

/* Call when the selected captive first attempts to allocate its own model.
 * Until then the fractured crystal keeps its body, even while invisible.
 * Either allocation outcome releases the old model slot. */
void bfm_village_mine_lake_fields_resident_allocation(BfmVillageMineLakeFields *fields,
                                                      unsigned resident_id);

/* The return includes an inside nudge. Ordinary airborne motion clears local
 * speed only on a fresh face entry, reported separately in airborne_entry. */
int bfm_village_mine_lake_fields_body_contact(const BfmVillageMineLakeFields *fields,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne,
    int *airborne_entry);

/* Query the sealed fields before their actor update. Return the resident ID and
 * intersection of a wall-clipped, accepted blade segment. The room owns the
 * subsequent mode-1 release script and the eventual rescue registration. */
unsigned bfm_village_mine_lake_fields_hit(const BfmVillageMineLakeFields *fields,
    const BfmVillageGround *ground, const BfmWoodsCasterMeleeFrame *swing, int16_t hit[3]);
/* The caller clips the segment against terrain and earlier actors. An accepted
 * hit shortens it so later actors see the same endpoint. */
unsigned bfm_village_mine_lake_fields_blade(BfmVillageMineLakeFields *fields, unsigned event,
    const int16_t from[3], int16_t to[3], int16_t normal[3]);
int bfm_village_mine_lake_fields_service(BfmVillageMineLakeFields *fields, BfmGameplayRun *run,
    const int16_t player[3], const int32_t eye[3], unsigned tick, const MusashiDiscMedia *disc);
/* Two entries in mounted placement order, valid until the next service. */
const BfmVillageMineLakeFieldEvent *bfm_village_mine_lake_fields_events(const BfmVillageMineLakeFields *fields);
int bfm_village_mine_lake_fields_busy(const BfmVillageMineLakeFields *fields);
void bfm_village_mine_lake_fields_claim_body(BfmVillageMineLakeFields *fields, unsigned id);
int bfm_village_mine_lake_fields_draw(void *fields, BfmBedroomFigure *figure,
                                     const BfmBedroomView *view);
#endif

/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ILLNESS_WINDMILL_H
#define BFM_VILLAGE_ILLNESS_WINDMILL_H
#include "bedroom_figure.h"
#include "bedroom_record.h"

typedef struct BfmVillageIllnessWindmill BfmVillageIllnessWindmill;

/* New commands issued together by Wid’s porch script during one field.
 * show is +1 to show, -1 to hide. door requests 1 to open and 2 to close.
 * Durations are in fields, turn is relative yaw, and walking speed is Q8. */
typedef struct {
    int show;
    unsigned door;
    uint32_t program;
    int turn;
    unsigned turn_fields;
    int walk_q8;
    unsigned walk_fields;
} BfmVillageWindmillCue;

BfmVillageIllnessWindmill *bfm_village_illness_windmill_open(const uint8_t *scene,
    size_t bytes, const BfmSaveRecord *save);
void bfm_village_illness_windmill_free(BfmVillageIllnessWindmill *windmill);
int bfm_village_illness_windmill_ready(const BfmVillageIllnessWindmill *windmill,
    const BfmSaveRecord *save, const int16_t player_xyz[3]);
int bfm_village_illness_windmill_step(BfmVillageIllnessWindmill *windmill,
    const BfmSaveRecord *save, int event_active, const BfmVillageWindmillCue *cue);
/* The script camera can target the resident's station while he is indoors. */
const int16_t *bfm_village_illness_windmill_position(const BfmVillageIllnessWindmill *windmill);
int bfm_village_illness_windmill_add(const BfmVillageIllnessWindmill *windmill,
    BfmBedroomFigure *figure, const BfmBedroomView *view);
/* Apply these to building 16 in the room's existing shop-door queue. */
/* The scene sound selector emitted on this actor field, or zero. */
unsigned bfm_village_illness_windmill_door_sound(const BfmVillageIllnessWindmill *windmill);
int bfm_village_illness_windmill_closed(const BfmVillageIllnessWindmill *windmill);
int bfm_village_illness_windmill_hinged(const BfmVillageIllnessWindmill *windmill);
int bfm_village_illness_windmill_door_angle(const BfmVillageIllnessWindmill *windmill);

#endif

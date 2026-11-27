/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LARDWICK_FIELD_H
#define BFM_WOODS_LARDWICK_FIELD_H

#include "bedroom_travel.h"
#include "gameplay_run.h"

typedef struct {
    BfmBedroomRoot root;
    unsigned light_pitch, light_yaw, detector_step;
    int active, lights_active, flash;
} BfmWoodsLardwickField;

/* Mount the field that holds Soldier Lardwick. The saved rescue variable
 * suppresses it on a later room entry. */
int bfm_woods_lardwick_field_open(BfmWoodsLardwickField *field, const uint8_t *scene,
                                 size_t bytes, const uint8_t variables[256]);
/* One eligible actor update, before the woods enemies use the same random stream. */
void bfm_woods_lardwick_field_step(BfmWoodsLardwickField *field, BfmGameplayRun *run,
                                  const int16_t player[3], unsigned field_tick);
#endif

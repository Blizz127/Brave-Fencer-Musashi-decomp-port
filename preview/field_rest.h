/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_REST_H
#define BFM_FIELD_REST_H

#include "bedroom_record.h"
#include "field_input.h"

typedef enum {
    BFM_FIELD_REST_FREE, BFM_FIELD_REST_CHARGE, BFM_FIELD_REST_DROP,
    BFM_FIELD_REST_ASLEEP, BFM_FIELD_REST_WAKE
} BfmFieldRestPose;

typedef struct {
    BfmFieldRestPose pose;
    unsigned ticks;
} BfmFieldRest;

/* Apply one ordinary outdoor input sample before advancing Musashi's pose.
 * Selected mode and charge live in the same record used by the field HUD. */
void bfm_field_rest_input(BfmFieldRest *rest, BfmSaveRecord *record, BfmFieldInput input);
void bfm_field_rest_advance(BfmFieldRest *rest, int pose_finished);
int bfm_field_rest_sleeping(const BfmFieldRest *rest);
void bfm_field_rest_stamina(BfmSaveRecord *record, int asleep);

#endif

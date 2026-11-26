/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_IMPACT_H
#define BFM_FOREST_IMPACT_H

#include "forest_field.h"

typedef struct {
    const uint8_t *descriptor, *cells;
    unsigned count;
} BfmForestImpactAnimation;

typedef struct {
    BfmForestImpactAnimation burst, strip;
    int32_t origin[3], burst_origin[3];
    BfmForestTransform face;
    int16_t strip_rotation[3][3];
    unsigned cell;
} BfmForestImpact;

/* Issue once for a confirmed grounded Fusion strike. Contact and its outward
 * Q12 normal come from the resolved hit surface. The room owns the borrowed
 * animation records and must outlive the effect. Cell zero draws immediately. */
int bfm_forest_impact_start(BfmForestImpact *impact, const BfmForestField *field,
                            const int16_t contact[3], const int16_t normal[3],
                            const int32_t camera_eye[3]);
void bfm_forest_impact_advance(BfmForestImpact *impact);
unsigned bfm_forest_impact_project(const BfmForestImpact *impact, const BfmForestCamera *camera,
                                   BfmForestDustCard out[BFM_FOREST_IMPACT_CARDS]);
#endif

/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MAYOR_CAST_H
#define BFM_VILLAGE_MAYOR_CAST_H

#include "bedroom_figure.h"

typedef struct BfmVillageMayorCast BfmVillageMayorCast;

/* Borrow the mounted town field and the payload of the Mayor's event member. */
BfmVillageMayorCast *bfm_village_mayor_cast_load(const uint8_t *scene, size_t bytes,
    uint32_t room, const uint8_t *mayor, size_t mayor_bytes, uint32_t shadow);
void bfm_village_mayor_cast_free(BfmVillageMayorCast *cast);

/* Age counts 30 Hz updates from the Square edge. Release age starts at zero
 * when the final window clears. The actor disappears after walking indoors. */
void bfm_village_mayor_cast_step(BfmVillageMayorCast *cast, unsigned age,
                                 unsigned release_age, int active, int returning);
const int16_t *bfm_village_mayor_cast_position(const BfmVillageMayorCast *cast);
int bfm_village_mayor_cast_add(const BfmVillageMayorCast *cast, BfmBedroomFigure *figure,
                               const BfmBedroomView *view);

#endif

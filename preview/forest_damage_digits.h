/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_DAMAGE_DIGITS_H
#define BFM_FOREST_DAMAGE_DIGITS_H
#include "forest_field.h"

typedef struct {
    int16_t contact[3];
    uint16_t amount;
    uint8_t age, palette;
} BfmForestDamageDigits;

/* Start the normal damage number at the accepted world hit point. Advance
 * existing numbers once per simulation update before issuing a new one.
 * Construction has no visible geometry. */
int bfm_forest_damage_digits_start(BfmForestDamageDigits *digits, const BfmForestField *field,
                                   const int16_t contact[3], uint16_t amount);
void bfm_forest_damage_digits_advance(BfmForestDamageDigits *digits);
unsigned bfm_forest_damage_digits_project(const BfmForestDamageDigits *digits,
                                         const BfmForestCamera *camera,
                                         BfmForestDustCard out[BFM_FOREST_DAMAGE_DIGIT_CARDS]);
#endif

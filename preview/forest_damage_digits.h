/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_DAMAGE_DIGITS_H
#define BFM_FOREST_DAMAGE_DIGITS_H
#include "forest_field.h"

typedef struct {
    int16_t contact[3];
    uint16_t amount;
    uint8_t age, palette;
    uint8_t hurt;
    int8_t drift_x, drift_z; /* Half world units per update. */
} BfmForestDamageDigits;

/* Start the normal damage number at the accepted world hit point. Advance
 * existing numbers once per simulation update before issuing a new one.
 * Construction has no visible geometry. */
int bfm_forest_damage_digits_start(BfmForestDamageDigits *digits, const BfmForestField *field,
                                   const int16_t contact[3], uint16_t amount);
/* The normal damage palette selector from the mounted room. */
int bfm_forest_damage_digits_start_data(BfmForestDamageDigits *digits, const uint8_t *palette,
                                        const int16_t contact[3], uint16_t amount);
/* First incoming rifle hit. The field effect dispatcher supplies its one
 * random result and the unshifted hero root. Signed amounts use up to three
 * numeral cards and a minus sign. */
int bfm_forest_damage_digits_start_hurt(BfmForestDamageDigits *digits, const BfmForestField *field,
                                        const int16_t hero[3], uint16_t amount, uint16_t random_result);
void bfm_forest_damage_digits_advance(BfmForestDamageDigits *digits);
int bfm_forest_damage_digits_prepare_hurt(BfmForestDamageDigits *digits, const BfmForestField *field,
                                          const int16_t hero[3], uint16_t amount);
void bfm_forest_damage_digits_scatter(BfmForestDamageDigits *digits, uint16_t random_result);
int bfm_forest_damage_digits_last(const BfmForestDamageDigits *digits);
unsigned bfm_forest_damage_digits_project(const BfmForestDamageDigits *digits,
                                         const BfmForestCamera *camera,
                                         BfmForestDustCard out[BFM_FOREST_DAMAGE_DIGIT_CARDS]);
#endif

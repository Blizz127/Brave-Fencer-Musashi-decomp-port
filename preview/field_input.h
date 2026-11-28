/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_INPUT_H
#define BFM_FIELD_INPUT_H

#include <stdint.h>

typedef struct {
    int8_t x, y; /* Filtered console stick bytes centered on 128. */
    uint8_t sampled;
} BfmChargeDirection;

typedef enum {
    BFM_FIELD_PAD_UNSPECIFIED,
    BFM_FIELD_PAD_DIGITAL,
    BFM_FIELD_PAD_ANALOG
} BfmFieldPadMode;

/* Ordinary movement axes range from -127 to 127. Square, Triangle and Circle are fresh presses.
 * Cross carries both the initial press and the held state for a jump.
 * Negative/positive camera direction is held L2/R2; either new press toggles the bank zoom. */
typedef struct {
    int x, y;
    int square_pressed, shoulder_held, camera_turn;
    int circle_pressed, cross_pressed, cross_held, triangle_pressed;
    unsigned held_faces;
    unsigned pressed_directions; /* Fresh wire-order D-pad bits for shop lists. */
    int camera_pressed;
    int select_pressed, start_pressed;
    BfmChargeDirection charge_direction; /* Filtered physical sample, including neutral. */
    BfmFieldPadMode movement_pad; /* Effective field pad after directional overrides. */
} BfmFieldInput;

static inline BfmChargeDirection bfm_field_charge_direction(BfmFieldInput input)
{
    if (input.charge_direction.sampled) return input.charge_direction;
    /* Scripted field inputs already supply normalized directions. */
    return (BfmChargeDirection){(int8_t)input.x, (int8_t)input.y, 0};
}

#endif

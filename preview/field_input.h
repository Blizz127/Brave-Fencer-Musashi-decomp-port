/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_INPUT_H
#define BFM_FIELD_INPUT_H

/* Stick axes range from -127 to 127. Square, Triangle and Circle are fresh presses.
 * Cross carries both the initial press and the held state for a jump.
 * Negative/positive camera direction is held L2/R2; either new press toggles the bank zoom. */
typedef struct {
    int x, y;
    int square_pressed, shoulder_held, camera_turn;
    int circle_pressed, cross_pressed, cross_held, triangle_pressed;
    unsigned held_faces;
    unsigned pressed_directions; /* Fresh wire-order D-pad bits for shop lists. */
    int camera_pressed;
    int select_pressed;
} BfmFieldInput;

#endif

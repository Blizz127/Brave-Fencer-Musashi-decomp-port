/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_INPUT_H
#define BFM_FIELD_INPUT_H

/* Stick axes range from -127 to 127. Square is an edge, R1 is a hold. */
typedef struct {
    int x, y;
    int square_pressed, shoulder_held;
} BfmFieldInput;

#endif

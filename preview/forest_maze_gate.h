/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_GATE_H
#define BFM_FOREST_MAZE_GATE_H

#include <stddef.h>
#include <stdint.h>

/* All pointers borrow the active room's unpacked scene. Copy the complete
 * 36-byte outgoing record before releasing it. The arrival row belongs to
 * the receiving room and supplies the incoming foot and facing. */
typedef struct {
    const uint8_t *arrival;
    const uint8_t *destination;
    const uint8_t *actor;
    const uint8_t *bounds;
} BfmForestMazeGate;

int bfm_forest_maze_bank_gate(BfmForestMazeGate *gate, const uint8_t *scene,
                              size_t size, unsigned entrance, unsigned story);
int bfm_forest_maze_return_gate(BfmForestMazeGate *gate, const uint8_t *scene,
                                size_t size, unsigned entrance);

/* Call with the preceding movement sweep's contact before beginning the
 * next controlled field. The vertical foot segment must meet this actor. */
const uint8_t *bfm_forest_maze_gate_contact(const BfmForestMazeGate *gate,
                                           unsigned wall_contact,
                                           const int16_t feet[3]);

/* Apply the source departure and receiving actor setup only after the new
 * visit has mounted successfully. The incoming setup returns a sound ID
 * when a later-story return to this clearing repeats its original cue. */
void bfm_forest_maze_gate_depart(const BfmForestMazeGate *gate,
                                uint8_t flags[64], uint8_t variables[256]);
unsigned bfm_forest_maze_gate_arrive(const BfmForestMazeGate *gate, uint8_t flags[64]);

#endif

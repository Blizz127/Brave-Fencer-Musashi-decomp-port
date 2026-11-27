/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ROTOR_H
#define BFM_VILLAGE_ROTOR_H

#include "gameplay_run.h"
#include "village_rotor_mesh.h"

typedef struct {
    BfmVillageRotorMesh *mesh;
    unsigned yaw, speed, remaining, phase;
} BfmVillageRotor;

/* Load on admission to the upper village. Clear before replacing the room. */
int bfm_village_rotor_load(BfmVillageRotor *rotor, const uint8_t *scene,
                          size_t bytes, unsigned story_progress);
void bfm_village_rotor_clear(BfmVillageRotor *rotor);
/* First step is the field update following admission, including fade-in.
 * Continue while the field's actors are running, even if the rotor is offscreen. */
void bfm_village_rotor_step(BfmVillageRotor *rotor, BfmGameplayRun *run, unsigned story_progress);
int bfm_village_rotor_append(const BfmVillageRotor *rotor, BfmBedroomFigure *figure,
                            const BfmBedroomView *view);

#endif

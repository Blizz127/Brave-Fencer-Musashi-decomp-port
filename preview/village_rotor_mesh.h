/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ROTOR_MESH_H
#define BFM_VILLAGE_ROTOR_MESH_H

#include "bedroom_figure.h"

typedef struct BfmVillageRotorMesh BfmVillageRotorMesh;

/* Load the upper village's placed four-vane rotor from SC03 member 2. */
BfmVillageRotorMesh *bfm_village_rotor_mesh_load(const uint8_t *scene, size_t bytes);
void bfm_village_rotor_mesh_free(BfmVillageRotorMesh *mesh);

/* Submit to the prepared field figure queue before drawing ground or actors.
 * yaw is the accumulated turn relative to the mounted placement heading. */
int bfm_village_rotor_mesh_append(const BfmVillageRotorMesh *mesh, BfmBedroomFigure *figure,
                                 const BfmBedroomView *view, unsigned yaw);

#endif

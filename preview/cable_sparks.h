/* SPDX-License-Identifier: MIT */
#ifndef BFM_CABLE_SPARKS_H
#define BFM_CABLE_SPARKS_H
#include "bedroom_field.h"
#include "forest_dust.h"
#include "forest_pose.h"
#include "gameplay_run.h"

typedef struct BfmCableSparks BfmCableSparks;
typedef struct {
    int depth;
    unsigned count;
    BfmForestRasterVertex vertices[4];
} BfmCableSparkFace;

/* Each fork uses four mounted pitch/roll pairs. The caller owns the
 * contact's world origin, rotational basis and field-effect slot lifetime. */
unsigned bfm_field_spark_fork(const BfmForestTransform *basis, const uint8_t *orientations,
                              size_t size, const int16_t origin[3], unsigned age,
                              const BfmBedroomView *view, BfmForestDustCard out[4]);
/* Supply one shared random draw at a new trail's birth. Its projected
 * segment lasts from age one through five, including its initial point. */
void bfm_field_spark_velocity(const BfmForestTransform *basis, uint16_t random,
                              int16_t velocity[3]);
int bfm_field_spark_trail(const int16_t origin[3], const int16_t velocity[3], unsigned age,
                          const BfmBedroomView *view, BfmForestDustCard *out);

/* Four little-endian pitch/roll pairs from the mounted cable scene. */
BfmCableSparks *bfm_cable_sparks_open(const uint8_t *orientations, size_t size);
BfmCableSparks *bfm_cable_sparks_open_yaw(const uint8_t *orientations, size_t size, unsigned yaw);
/* Advance existing particles every update. Emit on the scene's eligible
 * four-update clock phase, drawing both trail directions from the shared run. */
void bfm_cable_sparks_advance(BfmCableSparks *sparks, unsigned movement, int emit,
                              const int16_t position[3], BfmGameplayRun *run);
void bfm_cable_sparks_free(BfmCableSparks *sparks);
/* Advance once per movement update, with the rider's world-space root. */
void bfm_cable_sparks_step(BfmCableSparks *sparks, unsigned movement,
                           const int16_t position[3], BfmGameplayRun *run);
unsigned bfm_cable_sparks_project(BfmCableSparks *sparks, const BfmBedroomView *view,
                                  BfmCableSparkFace out[8]);
void bfm_cable_sparks_draw(const BfmForestRaster *surface, const BfmCableSparkFace *face);
#endif

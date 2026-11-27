/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_NOTICE_H
#define BFM_WOODS_ROOTRICK_NOTICE_H

#include "bedroom_figure.h"
#include "forest_marker.h"

/* Consume the queued notice on the player's next actor visit. The scene
 * remains mounted for the marker's lifetime. On each subsequent visit of
 * the effect actor, call bfm_forest_marker_advance with Musashi's feet. */
int bfm_woods_rootrick_notice_start(BfmForestMarker *marker,
                                    const uint8_t *scene, size_t bytes,
                                    const int16_t feet[3]);

/* Append the projected notice before draining the field's actor queue. */
int bfm_woods_rootrick_notice_add(const BfmForestMarker *marker, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view);

#endif

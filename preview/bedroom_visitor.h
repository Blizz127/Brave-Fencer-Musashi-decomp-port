/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_VISITOR_H
#define BFM_BEDROOM_VISITOR_H
#include "bedroom_figure.h"
#include "bedroom_travel.h"

typedef struct BfmBedroomVisitor BfmBedroomVisitor;

/* Keep the mounted scene alive until the visitor models are freed. */
BfmBedroomVisitor *bfm_bedroom_visitor_load(const uint8_t *scene, size_t size,
                                             const uint32_t *tables, unsigned count, uint32_t shadow_address);
void bfm_bedroom_visitor_free(BfmBedroomVisitor *visitor);

/* Append Ribson (0) or Livers (1) after preparing the figure queue and before
 * drawing the field. Each mounted pose has one eight-byte record per part. */
int bfm_bedroom_visitor_add(const BfmBedroomVisitor *visitor, BfmBedroomFigure *figure,
                           const BfmBedroomView *view, unsigned index,
                           const int position[3], int heading, uint32_t pose_address);
/* Scale local X/Z together and Y independently around the standing root.
 * The floor shadow keeps its normal size. A zero horizontal scale submits
 * only that shadow, without requiring a body pose. A supplied ground probe
 * determines its contact point, orientation and visibility; NULL retains
 * the ordinary visitor's level-ground shadow. */
int bfm_bedroom_visitor_add_scaled(const BfmBedroomVisitor *visitor, BfmBedroomFigure *figure,
                           const BfmBedroomView *view, unsigned index,
                           const int position[3], int heading, uint32_t pose_address,
                           unsigned horizontal_q12, unsigned vertical_q12, const BfmBedroomRoot *ground);
#endif

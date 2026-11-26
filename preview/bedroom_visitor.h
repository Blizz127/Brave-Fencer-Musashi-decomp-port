/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_VISITOR_H
#define BFM_BEDROOM_VISITOR_H
#include "bedroom_figure.h"

typedef struct BfmBedroomVisitor BfmBedroomVisitor;

/* Keep the mounted scene alive until the visitor models are freed. */
BfmBedroomVisitor *bfm_bedroom_visitor_load(const uint8_t *scene, size_t size);
void bfm_bedroom_visitor_free(BfmBedroomVisitor *visitor);

/* Append Ribson (0) or Livers (1) after preparing the figure queue and before
 * drawing the field. Each mounted pose has one eight-byte record per part. */
int bfm_bedroom_visitor_add(const BfmBedroomVisitor *visitor, BfmBedroomFigure *figure,
                           const BfmBedroomView *view, unsigned index,
                           const int position[3], int heading, uint32_t pose_address);
#endif

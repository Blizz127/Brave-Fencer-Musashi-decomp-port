/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SIGN_MODEL_H
#define BFM_WOODS_SIGN_MODEL_H

#include "bedroom_field.h"

typedef struct BfmWoodsSignModel BfmWoodsSignModel;

BfmWoodsSignModel *bfm_woods_sign_model_load(const uint8_t *scene, size_t bytes, uint32_t room);
/* Read a placed two-sided board and its solid post from the mounted room. */
BfmWoodsSignModel *bfm_woods_sign_model_from_placement(const uint8_t *scene, size_t bytes,
                                                       uint32_t row, uint32_t kit);
void bfm_woods_sign_model_free(BfmWoodsSignModel *sign);
const int16_t *bfm_woods_sign_model_position(const BfmWoodsSignModel *sign);
/* Resolve one player foot movement against the placed post. A miss keeps
 * the requested endpoint unchanged. */
int bfm_woods_sign_model_slide(const BfmWoodsSignModel *sign,
                               const int16_t previous[3], int16_t endpoint[3]);
int bfm_woods_sign_model_prepare(BfmWoodsSignModel *sign, const BfmBedroomView *view,
                                 const uint16_t tiles[512][1024], uint32_t *pixels);
int bfm_woods_sign_model_next_depth(const BfmWoodsSignModel *sign);
void bfm_woods_sign_model_draw_to(BfmWoodsSignModel *sign, int depth);

#endif

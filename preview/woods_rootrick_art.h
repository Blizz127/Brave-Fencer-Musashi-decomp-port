/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_ART_H
#define BFM_WOODS_ROOTRICK_ART_H

#include "bedroom_figure.h"

typedef struct BfmWoodsRootrickArt BfmWoodsRootrickArt;

/* The caller keeps the SC02/11 field mounted. Rootrick's mesh and joint
 * animations come from its separate challenge-event member on the disc. */
BfmWoodsRootrickArt *bfm_woods_rootrick_art_open(const MusashiDiscMedia *disc,
                                                 const uint8_t *scene, size_t bytes);
void bfm_woods_rootrick_art_free(BfmWoodsRootrickArt *art);

/* Call while the challenge actor's model is bound, with its display root
 * and the number of field updates since its current animation began. */
int bfm_woods_rootrick_art_add(const BfmWoodsRootrickArt *art, BfmBedroomFigure *figure,
                               const BfmBedroomView *view, const int16_t xyz[3],
                               unsigned yaw, int pitch, uint32_t clip_address, unsigned elapsed);

/* The ground shadow remains an independent actor during Rootrick's final blink. */
int bfm_woods_rootrick_art_visible(const BfmWoodsRootrickArt *art, BfmBedroomFigure *figure,
                                   const BfmBedroomView *view, const int16_t xyz[3],
                                   unsigned yaw, int pitch, uint32_t clip, unsigned elapsed,
                                   int show_model, int show_shadow);
#endif

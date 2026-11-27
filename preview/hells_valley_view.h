/* SPDX-License-Identifier: MIT */
#ifndef BFM_HELLS_VALLEY_VIEW_H
#define BFM_HELLS_VALLEY_VIEW_H

#include "village_field.h"

struct BfmValleyKnitter;
typedef struct BfmHellsValleyView BfmHellsValleyView;

/* SC03 member 31 and the incoming tile pages remain mounted until free. */
BfmHellsValleyView *bfm_hells_valley_view_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, unsigned phase, uint16_t tiles[512][1024]);
void bfm_hells_valley_view_free(BfmHellsValleyView *view);
BfmVillageField *bfm_hells_valley_view_field(BfmHellsValleyView *view);
const BfmVillageCamera *bfm_hells_valley_view_submitted(const BfmHellsValleyView *view);
const BfmVillageCamera *bfm_hells_valley_view_camera(const BfmHellsValleyView *view);
const BfmBedroomView *bfm_hells_valley_projection(const BfmHellsValleyView *view);
void bfm_hells_valley_view_follow(BfmHellsValleyView *view, const int16_t foot[3],
                                  int entering, int shoulder_pressed, int shake_y);
void bfm_hells_valley_view_follow_captive(BfmHellsValleyView *view, const int16_t foot[3],
    int entering, int shoulder_pressed, int shake_y, struct BfmValleyKnitter *knitter);
void bfm_hells_valley_view_light(BfmHellsValleyView *view, unsigned phase);
int bfm_hells_valley_view_draw(BfmHellsValleyView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12, int hide_body,
    uint32_t pixels[240][320],
    int (*submit)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor);

#endif

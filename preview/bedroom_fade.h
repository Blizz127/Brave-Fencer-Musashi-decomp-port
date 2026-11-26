/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_FADE_H
#define BFM_BEDROOM_FADE_H

#include <stdint.h>

typedef struct {
    unsigned remaining, top, bottom;
    int visible;
} BfmBedroomFade;

/* Begin when the early-story actor launches the bedroom event. Advance once
 * per following scene update and draw over the completed room framebuffer. */
void bfm_bedroom_fade_begin(BfmBedroomFade *fade);
void bfm_bedroom_fade_step(BfmBedroomFade *fade);
void bfm_bedroom_fade_draw(const BfmBedroomFade *fade, uint32_t pixels[240][320]);

#endif

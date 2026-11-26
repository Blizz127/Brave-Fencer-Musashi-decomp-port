/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_LIGHTING_H
#define BFM_VILLAGE_LIGHTING_H

#include "bedroom_figure.h"

/* The daylight phase is the same five-way selection as the lower-town sky. */
int bfm_village_lower_light(BfmBedroomFigure *figure, const uint8_t *scene,
                            size_t bytes, unsigned phase);

typedef struct {
    const uint8_t *scene, *rectangles, *entry_colors, *transition_colors, *sky_colors, *lamp_colors;
    size_t count;
    BfmBedroomLight lamps[3];
    uint8_t sky[3];
    unsigned phase, blending, wait;
} BfmVillageDaylight;

/* The upper field owns its time-of-day palettes. The source colors in the
 * mounted scene remain available throughout a transition. */
int bfm_village_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                               uint32_t room, unsigned phase, uint16_t tiles[512][1024]);
void bfm_village_daylight_step(BfmVillageDaylight *light, unsigned phase, uint16_t tiles[512][1024]);
#endif

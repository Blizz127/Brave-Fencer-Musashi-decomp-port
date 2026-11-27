/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_LIGHTING_H
#define BFM_VILLAGE_LIGHTING_H

#include "bedroom_figure.h"

/* The daylight phase is the same five-way selection as the lower-town sky. */
int bfm_village_lower_light(BfmBedroomFigure *figure, const uint8_t *scene,
                            size_t bytes, unsigned phase);

/* Direct-color streets shade rectangular samples already mounted in video memory. */
typedef struct {
    const uint8_t *rows, *levels;
    unsigned count, phase, wait, gain[3];
    unsigned deferred, dirty;
} BfmStreetColors;

int bfm_village_street_colors_begin(BfmStreetColors *colors, const uint8_t *scene, size_t bytes,
                                    uint32_t room, unsigned phase, uint16_t tiles[512][1024]);
void bfm_village_street_colors_step(BfmStreetColors *colors, unsigned phase, uint16_t tiles[512][1024]);

typedef struct {
    BfmStreetColors street;
    const uint8_t *scene, *rectangles, *entry_colors, *transition_colors, *sky_colors, *lamp_colors;
    size_t count;
    BfmBedroomLight lamps[3];
    uint8_t sky[3], next_sky[3];
    unsigned phase, blending, wait;
} BfmVillageDaylight;

/* Each field supplies its own palette rectangles and color tables. Keep the
 * mounted scene available throughout a transition. */
int bfm_village_illness_light_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                   unsigned phase, uint16_t tiles[512][1024]);
int bfm_village_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                               uint32_t room, unsigned phase, uint16_t tiles[512][1024]);
int bfm_woods_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                             unsigned phase, uint16_t tiles[512][1024]);
int bfm_hells_valley_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                    unsigned phase, uint16_t tiles[512][1024]);
int bfm_forest_bank_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                   unsigned phase, uint16_t tiles[512][1024]);
int bfm_forest_maze_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                   unsigned phase, uint16_t tiles[512][1024]);
void bfm_village_daylight_step(BfmVillageDaylight *light, unsigned phase, uint16_t tiles[512][1024]);
#endif

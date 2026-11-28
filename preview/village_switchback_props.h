/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_SWITCHBACK_PROPS_H
#define BFM_VILLAGE_SWITCHBACK_PROPS_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

typedef struct BfmVillageSwitchbackProps BfmVillageSwitchbackProps;

/* Keep the western switchback scene mounted until the props are freed. */
BfmVillageSwitchbackProps *bfm_village_switchback_props_open(const uint8_t *scene, size_t bytes,
    const int16_t hero[3], unsigned world_tick);
void bfm_village_switchback_props_free(BfmVillageSwitchbackProps *props);
int bfm_village_switchback_props_step(BfmVillageSwitchbackProps *props, const int16_t hero[3],
    unsigned world_tick, BfmGameplayRun *run, int suppress_tracking);
int bfm_village_switchback_props_draw(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif

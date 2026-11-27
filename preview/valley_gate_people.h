/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_GATE_PEOPLE_H
#define BFM_VALLEY_GATE_PEOPLE_H
#include "bedroom_figure.h"

typedef struct BfmValleyGatePeople BfmValleyGatePeople;

/* The mounted Valley scene must remain available for the duration of the visit.
 * The four attendants are admitted only on entry at story 150. */
BfmValleyGatePeople *bfm_valley_gate_people_open(const uint8_t *scene, size_t bytes,
                                                 unsigned entry_story, uint16_t tiles[512][1024]);
void bfm_valley_gate_people_free(BfmValleyGatePeople *people);
/* Signals are in Soldier, Carpenter, Mercenary, Knight order. Each child clears
 * its switch instruction on receipt. The Soldier's landing wakes the gate. */
int bfm_valley_gate_people_step(BfmValleyGatePeople *people, uint8_t member[4]);
int bfm_valley_gate_people_draw(const BfmValleyGatePeople *people, BfmBedroomFigure *figure,
                                const BfmBedroomView *view);
#endif

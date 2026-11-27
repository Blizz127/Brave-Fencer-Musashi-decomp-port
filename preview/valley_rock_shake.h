/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_ROCK_SHAKE_H
#define BFM_VALLEY_ROCK_SHAKE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BFM_VALLEY_SHAKE_EARTH_STRIKE,
    BFM_VALLEY_SHAKE_ROLL,
    BFM_VALLEY_SHAKE_LANDING,
    BFM_VALLEY_SHAKE_GATE
} BfmValleyShakeEvent;

typedef struct {
    const uint8_t *earth, *roll, *landing, *gate;
    const uint8_t *next;
    unsigned hold;
    int world_y;
} BfmValleyRockShake;

/* The mounted field and MAIN/10 payloads must outlive the room. */
int bfm_valley_rock_shake_load(BfmValleyRockShake *shake, const uint8_t *scene,
    size_t scene_bytes, const uint8_t *main, size_t main_bytes);
void bfm_valley_rock_shake_request(BfmValleyRockShake *shake, BfmValleyShakeEvent event);

/* Once per unpaused field update, after requests. Add the returned displacement
 * to both world-space camera Y coordinates for this field's draw only. */
int bfm_valley_rock_shake_step(BfmValleyRockShake *shake);

#endif

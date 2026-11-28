/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_SHAFT_FAN_H
#define BFM_VILLAGE_MINE_SHAFT_FAN_H

#include "bedroom_figure.h"
#include "village_mine_basket.h"

typedef struct BfmMineShaftFan BfmMineShaftFan;
enum { BFM_MINE_SHAFT_FAN_PIECES = 5 };

typedef struct {
    int carry_yaw;
    unsigned sound_count;
    struct { unsigned command, value; } sound[2];
} BfmMineShaftFanFrame;

/* Keep the mounted shaft scene alive through free. The hub draws the whole fan. */
BfmMineShaftFan *bfm_mine_shaft_fan_open(const uint8_t *scene, size_t bytes);
void bfm_mine_shaft_fan_free(BfmMineShaftFan *fan);

/* The last completed shapes in contact order: hub 0, four bearings 1..4. */
const BfmForestTransform *bfm_mine_shaft_fan_shape(const BfmMineShaftFan *fan,
    unsigned piece, const BfmForestGuardBox **box);

/* The ordinary player tests its ankles before its supporting foot. The
 * airborne mask suppresses the horizontal push on a vertical face entry. */
enum { BFM_SHAFT_BODY_GROUND, BFM_SHAFT_BODY_AIR, BFM_SHAFT_BODY_AIR_NO_FACE_PUSH };
/* Ground returns bit 0 for a face entry and bit 1 for an interior start.
 * Air returns 1 for entry. Only AIR_NO_FACE_PUSH also reports interior
 * contact, although both airborne modes can move the foot out of a box. */
int bfm_mine_shaft_fan_body(const BfmMineShaftFan *fan, int rider,
    const int16_t previous[3], int16_t foot[3], unsigned tick, int mode);
BfmMineBasketContact bfm_mine_shaft_fan_probe(const BfmMineShaftFan *fan,
    const int16_t previous[3], const int16_t candidate[3], const int16_t last_supported[3],
    int *rider, int16_t foot[3]);
void bfm_mine_shaft_fan_shadow(const BfmMineShaftFan *fan,
    const int16_t carried[3], BfmBedroomRoot *shadow);

/* After ordinary movement, update the hub, wind, then bearings. rider is the
 * support selected by that movement (-1 for none). A mounted rider turns by
 * carry_yaw. Action 2/3 preserves the existing vertical velocity in the wind.
 * A null player can advance the actor clocks without applying motion. */
BfmMineShaftFanFrame bfm_mine_shaft_fan_step(BfmMineShaftFan *fan,
    unsigned player_action, int rider, int32_t foot_q16[3], int32_t *vertical_q16);
int bfm_mine_shaft_fan_draw(void *fan, BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif

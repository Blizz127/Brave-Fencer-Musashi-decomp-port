/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ILLNESS_MINE_GATE_H
#define BFM_VILLAGE_ILLNESS_MINE_GATE_H

#include "bedroom_figure.h"
#include "forest_message.h"
#include "gameplay_run.h"

typedef struct BfmVillageMineGate BfmVillageMineGate;
typedef enum {
    BFM_MINE_GATE_ERROR = -1, BFM_MINE_GATE_FREE, BFM_MINE_GATE_BUSY, BFM_MINE_GATE_RELEASED
} BfmMineGateResult;

typedef enum { BFM_MINE_GATE_WALK, BFM_MINE_GATE_TURN, BFM_MINE_GATE_POSE } BfmMineGateOrderType;
typedef struct {
    BfmMineGateOrderType type;
    int16_t x, z;
    unsigned yaw;
    uint32_t body_program;
} BfmMineGateOrder;

typedef struct {
    BfmForestMessage *opened; /* Ownership transfers to the field window manager. */
    const uint8_t *camera_station, *camera_frame;
    int release_camera;
    unsigned sound;
    BfmMineGateOrder orders[2]; /* Dispatch in order through the ordinary scripted player. */
    unsigned order_count;
} BfmMineGateFrame;

/* Borrows the illness-era lower Grillin scene. A restored unlocked gate starts open. */
BfmVillageMineGate *bfm_village_mine_gate_open(const uint8_t *scene, size_t bytes,
                                               const BfmSaveRecord *save);
void bfm_village_mine_gate_free(BfmVillageMineGate *gate);
int bfm_village_mine_gate_active(const BfmVillageMineGate *gate);

/* Call on the gate actors' turn, after the player and earlier interactions.
 * can_interact requires ordinary standing or walking with control. A fresh
 * Square edge selects the lock. On window close, pass its actual answer:
 * 1 chooses the key, 2 tries Lumina before choosing the key. The final
 * release waits for the queued player orders and camera return. */
BfmMineGateResult bfm_village_mine_gate_step(BfmVillageMineGate *gate, const int16_t foot[3],
    int can_interact, int square_edge, int page_closed, unsigned answer,
    int player_ready, int camera_restored, BfmSaveRecord *save, BfmGameplayRun *run,
    BfmMineGateFrame *frame);

/* Apply both rotating actor boxes to the proposed foot before sweeping terrain. */
int bfm_village_mine_gate_block_body(const BfmVillageMineGate *gate, const int16_t from[3],
                                     int16_t to[3], unsigned tick, int airborne);
/* Queue both actual disc meshes with the other field actors. */
int bfm_village_mine_gate_draw(const BfmVillageMineGate *gate, BfmBedroomFigure *figure,
                               const BfmBedroomView *view);
/* A crossed mine threshold supplies a borrowed 36-byte exit once control is
 * free and both leaves have finished opening. */
const uint8_t *bfm_village_mine_gate_passage(const BfmVillageMineGate *gate, unsigned material);
#endif

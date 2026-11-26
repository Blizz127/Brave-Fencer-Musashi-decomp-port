/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_SCRIPT_H
#define BFM_BEDROOM_SCRIPT_H

#include <stddef.h>
#include <stdint.h>

/* A command carries a camera/text address, player event selector, sound
 * request or movement operand, depending on the high-level game opcode. */
typedef enum {
    BFM_BED_CAMERA_CUT, BFM_BED_CAMERA_MOVE, BFM_BED_LIE, BFM_BED_RISE,
    BFM_BED_UPRIGHT, BFM_BED_YAWN, BFM_BED_CUE, BFM_BED_TURN,
    BFM_BED_WALK, BFM_BED_RETREAT, BFM_BED_NOTICE, BFM_BED_VISITORS_ENTER,
    BFM_BED_RIBSON_SPEAKS, BFM_BED_RIBSON_RESTS, BFM_BED_BUTLER_SPEAKS,
    BFM_BED_VISITOR_LAYOUT, BFM_BED_PLAYER_LAYOUT, BFM_BED_WINDOW_POSITION,
    BFM_BED_WINDOW, BFM_BED_VISITOR_WINDOW
} BfmBedroomEvent;
typedef struct {
    const uint8_t *scene;
    size_t size, cursor;
    unsigned delay;
    int waiting_for_player, waiting_for_message, failed;
} BfmBedroomScript;
typedef int (*BfmBedroomApply)(void *context, BfmBedroomEvent event, uint32_t operand);
int bfm_bedroom_script_begin(BfmBedroomScript *script, const uint8_t *scene, size_t bytes);
int bfm_bedroom_script_tick(BfmBedroomScript *script, int player_queue_idle, int message_active,
                            BfmBedroomApply apply, void *context);
#endif

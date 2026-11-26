/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_VISITORS_H
#define BFM_BEDROOM_VISITORS_H

#include <stddef.h>
#include <stdint.h>

typedef struct BfmBedroomVisitors BfmBedroomVisitors;
typedef enum {
    BFM_VISITOR_RIBSON_SPEAK,
    BFM_VISITOR_RIBSON_REST,
    BFM_VISITOR_BUTLER_SPEAK
} BfmBedroomVisitorCue;

typedef struct {
    int position[3], heading;
    uint32_t pose_address;
    unsigned visible;
} BfmBedroomVisitorPose;

/* The mounted SC01/6 payload remains owned by the caller. */
BfmBedroomVisitors *bfm_bedroom_visitors_open(const uint8_t *scene, size_t bytes);
int bfm_bedroom_visitors_spawn(BfmBedroomVisitors *visitors);
void bfm_bedroom_visitors_request(BfmBedroomVisitors *visitors, BfmBedroomVisitorCue cue);
void bfm_bedroom_visitors_speak(BfmBedroomVisitors *visitors);
int bfm_bedroom_visitors_step(BfmBedroomVisitors *visitors);
const BfmBedroomVisitorPose *bfm_bedroom_visitors_pose(const BfmBedroomVisitors *visitors,
                                                       unsigned visitor);
void bfm_bedroom_visitors_free(BfmBedroomVisitors *visitors);
#endif

/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_TRAVEL_H
#define BFM_BEDROOM_TRAVEL_H

#include <stddef.h>
#include <stdint.h>

typedef struct BfmBedroomTravel BfmBedroomTravel;

typedef enum {
    BFM_TRAVEL_NONE, BFM_TRAVEL_WALK, BFM_TRAVEL_FALL, BFM_TRAVEL_LAND, BFM_TRAVEL_STAND
} BfmTravelEdge;

typedef struct {
    int16_t xyz[3];
    int16_t normal[3];
    unsigned yaw;
    unsigned busy;
    BfmTravelEdge locomotion_edge;
    int16_t shadow_xyz[3], shadow_normal[3];
    unsigned shadow_visible;
} BfmBedroomRoot;

/* The caller keeps the unpacked SC01 room member mounted for this lifetime. */
BfmBedroomTravel *bfm_bedroom_travel_open(const uint8_t *scene, size_t bytes);
void bfm_bedroom_travel_free(BfmBedroomTravel *travel);
const BfmBedroomRoot *bfm_bedroom_travel_root(const BfmBedroomTravel *travel);
void bfm_bedroom_travel_look(BfmBedroomTravel *travel, int yaw);
void bfm_bedroom_travel_face_point(BfmBedroomTravel *travel, int x, int z);
int bfm_bedroom_travel_walk(BfmBedroomTravel *travel, int16_t x, int16_t z);
int bfm_bedroom_travel_turn(BfmBedroomTravel *travel, unsigned yaw);
int bfm_bedroom_travel_step(BfmBedroomTravel *travel,
                            const int32_t eye[3], const int32_t target[3]);

#endif

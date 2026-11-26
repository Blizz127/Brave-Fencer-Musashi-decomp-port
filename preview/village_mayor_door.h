/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MAYOR_DOOR_H
#define BFM_VILLAGE_MAYOR_DOOR_H
#include "bedroom_field.h"

typedef struct BfmVillageMayorDoor BfmVillageMayorDoor;

/* Scene bytes and the room address refer to the mounted town field. */
BfmVillageMayorDoor *bfm_village_mayor_door_load(const uint8_t *scene, size_t bytes,
                                                 uint32_t room);
void bfm_village_mayor_door_free(BfmVillageMayorDoor *door);
int bfm_village_mayor_door_on_porch(const BfmVillageMayorDoor *door, const int16_t xyz[3]);
/* Ages count 30 Hz updates since the accepted Square and final dialogue clear. */
int bfm_village_mayor_door_angle(unsigned elapsed, int departing, unsigned release_age);
void bfm_village_mayor_door_prepare(BfmVillageMayorDoor *door, const BfmBedroomView *view,
                                    const uint16_t tiles[512][1024], uint32_t *pixels,
                                    int angle);
int bfm_village_mayor_door_next_depth(const BfmVillageMayorDoor *door);
void bfm_village_mayor_door_draw_to(BfmVillageMayorDoor *door, int depth);
#endif

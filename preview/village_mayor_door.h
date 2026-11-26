/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MAYOR_DOOR_H
#define BFM_VILLAGE_MAYOR_DOOR_H
#include "bedroom_field.h"
#include "bedroom_record.h"

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
/* Storefronts visible from the field return outside the inn. */
typedef struct BfmVillageShopDoors BfmVillageShopDoors;
BfmVillageShopDoors *bfm_village_shop_doors_load(const uint8_t *scene, size_t bytes, uint32_t room);
void bfm_village_shop_doors_free(BfmVillageShopDoors *doors);
void bfm_village_shop_doors_prepare(BfmVillageShopDoors *doors, const BfmBedroomView *view,
                                    const uint16_t tiles[512][1024], uint32_t *pixels,
                                    const BfmSaveRecord *state);
int bfm_village_shop_doors_next_depth(const BfmVillageShopDoors *doors);
void bfm_village_shop_doors_draw_to(BfmVillageShopDoors *doors, int depth);
#endif

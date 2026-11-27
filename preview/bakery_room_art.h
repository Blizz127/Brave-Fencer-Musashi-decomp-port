/* SPDX-License-Identifier: MIT */
#ifndef BFM_BAKERY_ROOM_ART_H
#define BFM_BAKERY_ROOM_ART_H

#include "bedroom_field.h"
#include "village_camera.h"
#include "bedroom_motion.h"
#include "bedroom_travel.h"

typedef struct BfmBakeryRoomArt BfmBakeryRoomArt;
extern const BfmVillageCamera bfm_bakery_counter_camera;

/* Mount the village and Jam's shop on entry. Keep the shared tile store alive
 * until the room is freed. Callers own the player's movement and poses. */
BfmBakeryRoomArt *bfm_bakery_room_art_open(const MusashiDiscMedia *disc,
                                          uint16_t tiles[512][1024]);
void bfm_bakery_room_art_free(BfmBakeryRoomArt *art);
const uint8_t *bfm_bakery_room_art_scene(const BfmBakeryRoomArt *art, size_t *size,
                                        uint32_t *room);
const BfmBedroomView *bfm_bakery_room_art_view(const BfmBakeryRoomArt *art);

/* Draw the room, the current player pose and Jam in a single depth queue.
 * The field layer draws the greeting, shopping panels and HUD afterwards. */
int bfm_bakery_room_art_draw(BfmBakeryRoomArt *art, const BfmBedroomRoot *root,
                            const BfmBedroomPose *pose, const uint8_t flags[64],
                            unsigned age, uint32_t pixels[240][320]);

#endif

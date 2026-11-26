/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_INTERIOR_BELL_H
#define BFM_TOWER_INTERIOR_BELL_H

#include "forest_terrain.h"
#include "forest_field.h"
#include "room_descriptor.h"

typedef struct BfmTowerInteriorBell BfmTowerInteriorBell;

/* Open the bell from the mounted US interior scene. The mesh and
 * placement remain valid after the caller releases the scene bytes. */
BfmTowerInteriorBell *bfm_tower_interior_bell_open(const uint8_t *scene, size_t size);
void bfm_tower_interior_bell_free(BfmTowerInteriorBell *bell);

/* Age zero is the bell's construction update. The returned actor borrows its
 * root and pose until the next sample or release. */
int bfm_tower_interior_bell_actor(BfmTowerInteriorBell *bell, unsigned elapsed_updates,
                                  BfmForestActor *out);

typedef enum {
    BFM_TOWER_BELL_PLAYER_CAMERA,
    BFM_TOWER_BELL_SIDE_CAMERA,
    BFM_TOWER_BELL_OVERHEAD_CAMERA
} BfmTowerBellCamera;

typedef struct {
    unsigned age, room_phase, phase_age, white;
    /* An edge consumed once per logical update; zero means no sound command. */
    unsigned sound_command, sound_argument;
    BfmTowerBellCamera camera;
    int locked, door_open, shake;
    int16_t door[3];
} BfmTowerBellResponse;

/* Supply the pre/post-movement world segment of a real projectile, clipped
 * to the room geometry. A miss leaves outputs and reaction state unchanged.
 * On the first accepted Gunshot the caller consumes the projectile and the
 * response starts at age zero. */
int bfm_tower_interior_bell_contact(BfmTowerInteriorBell *bell, unsigned room_age,
    unsigned attack_class, const int16_t from[3], const int16_t to[3],
    int16_t hit[3], int16_t normal[3]);

/* Advance once per scene update, before processing that update's newly
 * arriving projectiles. The completed reaction retains its opened gate. */
void bfm_tower_interior_bell_tick(BfmTowerInteriorBell *bell);
int bfm_tower_interior_bell_response(const BfmTowerInteriorBell *bell,
                                     BfmTowerBellResponse *response);
/* The destination belongs to the type-124 door that is created by the last
 * scene update. Returning it does not move the player through the door. */
const RoomTransferDescriptor *bfm_tower_interior_bell_destination(const BfmTowerInteriorBell *bell);
/* A landed floor contact selects the door receiver. Supply the player's
 * feet on the same update. No action button is needed. */
const RoomTransferDescriptor *bfm_tower_interior_bell_door_contact(
    const BfmTowerInteriorBell *bell, const BfmForestContact *support, const int16_t feet[3]);
#endif

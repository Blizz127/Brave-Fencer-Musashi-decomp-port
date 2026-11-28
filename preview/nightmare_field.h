/* SPDX-License-Identifier: MIT */
#ifndef BFM_NIGHTMARE_FIELD_H
#define BFM_NIGHTMARE_FIELD_H

#include "bedroom_field.h"
#include "bedroom_record.h"

/* Each recovery room mounts its own scene resources and return descriptor. */
typedef struct {
    uint16_t number;
    uint32_t street_exit, binding, script, camera;
    uint32_t figure, face, flash, landing_sounds, seated_body, message, window;
} BfmNightmareLayout;

const BfmNightmareLayout *bfm_nightmare_layout(const BfmBedroomEntry *entry);

/* Resolve entrance zero in the mounted SC03 recovery room. On failure all
 * outputs retain their prior values. */
int bfm_nightmare_entrance(const uint8_t *scene, size_t bytes, const BfmBedroomEntry *entry,
                           uint32_t *room_address, int16_t xyz[3], unsigned *yaw);

/* Load the field mesh and the initial camera looking at the arriving player.
 * The caller supplies the mounted room's textures when drawing the field,
 * and owns the returned field. On success, room_address identifies its mounted
 * room record for the player and floor decoders. */
BfmBedroomField *bfm_nightmare_field_open(const uint8_t *scene, size_t bytes,
                                          const BfmBedroomEntry *entry, uint32_t *room_address);

#endif

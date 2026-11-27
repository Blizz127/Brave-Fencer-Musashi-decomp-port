/* SPDX-License-Identifier: MIT */
#ifndef BFM_BAKERY_DOOR_H
#define BFM_BAKERY_DOOR_H

#include "bedroom_record.h"
#include <stddef.h>
#include <stdint.h>

int bfm_bakery_door_is_open(const BfmSaveRecord *save);
/* Borrow the lower-street bakery entry when the controlled player makes
 * contact with its open doorway. The caller owns scene and player state. */
const uint8_t *bfm_bakery_door_transfer(const uint8_t *scene, size_t bytes,
                                        uint16_t wall_contact, const BfmSaveRecord *save);

#endif

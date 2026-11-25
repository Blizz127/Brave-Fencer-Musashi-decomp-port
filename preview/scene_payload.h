/* SPDX-License-Identifier: MIT */
#ifndef BFM_SCENE_PAYLOAD_H
#define BFM_SCENE_PAYLOAD_H

#include "musashi_disc_media.h"

/* Expand a scene member's first compressed PAC section from the mounted
 * game. Both allocation budgets are enforced before reading the payload.
 * The caller owns the returned bytes. Failure sets *size to zero. */
uint8_t *bfm_scene_unpack_pac0(const MusashiDiscMedia *disc, const char *archive,
                               unsigned member, size_t packed_limit,
                               size_t output_limit, size_t *size);
#endif

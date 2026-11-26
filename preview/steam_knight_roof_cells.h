/* SPDX-License-Identifier: MIT */
#ifndef BFM_STEAM_KNIGHT_ROOF_CELLS_H
#define BFM_STEAM_KNIGHT_ROOF_CELLS_H

#include <stddef.h>
#include <stdint.h>

/* Apply the hall roof's four mounted replacement cells. The caller owns
 * the returned sparse mesh. A rejected input returns NULL and size zero. */
uint8_t *bfm_steam_knight_roof_mesh(const uint8_t *base_mesh, size_t base_size,
                                    const uint8_t *replacement, size_t replacement_size,
                                    size_t *out_size);

/* A gate replaces one adjacent horizontal or vertical pair of sectors. */
uint8_t *bfm_steam_knight_gate_mesh(const uint8_t *mesh, size_t bytes,
                                    const uint8_t *record, size_t size, size_t *out_size);

#endif

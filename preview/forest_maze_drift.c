/* SPDX-License-Identifier: MIT */
#include "forest_maze_drift.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

static const uint32_t origin = UINT32_C(0x80128158);

static const uint8_t *record(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (!scene || address < origin || address - origin > bytes || length > bytes - (address - origin))
        return NULL;
    return scene + (address - origin);
}

int bfm_forest_maze_drift_load(BfmForestDrift *drift, const uint8_t *scene,
                               size_t bytes, unsigned entrance)
{
    const uint8_t *room = record(scene, bytes, UINT32_C(0x8018c890), 0xbc);
    if (!drift || !room || (entrance != 0 && entrance != 33)) return 0;
    uint32_t list = entrance ? UINT32_C(0x8018ce04) : u32le(room + 0xb4);
    uint32_t end = entrance ? UINT32_C(0x8018ce90) : u32le(room + 0xb8);
    unsigned variant = 0;
    while (list < end && end - list >= 20) {
        const uint8_t *row = record(scene, bytes, list, 20);
        if (!row) return 0;
        unsigned kind = u16le(row + 6);
        if (!kind) {
            if (entrance == 33 && !variant) {
                memset(drift, 0, sizeof *drift);
                return 1;
            }
            return entrance == 0 && variant == 2 &&
                   bfm_forest_drift_init(drift, record(scene, bytes, UINT32_C(0x801e4d50), 32),
                                        record(scene, bytes, UINT32_C(0x801e4d78), 12), variant);
        }
        if (kind == 33) {
            if (variant || u16le(row + 8) != 2) return 0;
            variant = 2;
        }
        list += 20;
    }
    return 0;
}

unsigned bfm_forest_maze_drift_project(const BfmForestDrift *drift,
                                       BfmForestDustCard out[BFM_FOREST_MAZE_DRIFT_CARDS])
{
    return bfm_forest_drift_project_cards(drift, out, BFM_FOREST_MAZE_DRIFT_CARDS);
}

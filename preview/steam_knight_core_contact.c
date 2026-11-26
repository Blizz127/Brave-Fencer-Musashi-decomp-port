/* SPDX-License-Identifier: MIT */
#include "steam_knight_core_contact.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_steam_knight_core_boxes_load(BfmSteamKnightCoreBoxes *boxes,
                                     const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158), closed = UINT32_C(0x80196328);
    size_t offset = closed - base;
    if (!boxes || !scene || offset > bytes || bytes - offset < 44) return 0;
    const uint8_t *shapes = scene + offset;
    const uint8_t *registration = shapes + 32;
    if (u16le(registration) != 0 || u16le(registration + 2) != 0x8000 ||
        u32le(registration + 8) != closed) return 0;

    BfmSteamKnightCoreBoxes result;
    for (unsigned which = 0; which < 2; ++which) {
        const uint8_t *record = shapes + 16 * which;
        if (u32le(record) != 0) return 0;
        BfmForestGuardBox *box = which ? &result.opened : &result.closed;
        for (unsigned axis = 0; axis < 3; ++axis) {
            box->low[axis] = s16le(record + 4 + 4 * axis);
            box->high[axis] = s16le(record + 6 + 4 * axis);
            if (box->low[axis] >= box->high[axis]) return 0;
        }
    }
    *boxes = result;
    return 1;
}

int bfm_steam_knight_core_intersect(const BfmSteamKnightCoreBoxes *boxes,
                                    BfmSteamKnightCoreSurface surface,
                                    const BfmForestTransform *attachment,
                                    const int16_t start[3], const int16_t ends[][3],
                                    size_t count, BfmSteamKnightCoreContact *contact)
{
    if (!boxes || !attachment || !start || !ends || !contact ||
        (surface != BFM_STEAM_CLOSED_CAP && surface != BFM_STEAM_OPEN_CORE) ||
        count > BFM_FOREST_STRIKE_RAYS) return 0;
    const BfmForestGuardBox *shape = surface == BFM_STEAM_OPEN_CORE ? &boxes->opened : &boxes->closed;
    for (size_t ray = 0; ray < count; ++ray) {
        if (bfm_forest_guard_contact_point(shape, attachment, start, ends[ray],
                                           contact->center, contact->point, contact->normal))
            return 1;
    }
    return 0;
}

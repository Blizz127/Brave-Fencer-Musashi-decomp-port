/* SPDX-License-Identifier: MIT */
#include "village_mine_decks.h"

#include <string.h>

BfmMineBasketContact bfm_village_mine_decks_probe(const BfmVillageMineWheels *wheels,
    const int16_t previous[3], const int16_t candidate[3], const int16_t last_supported[3],
    int *rider, int16_t foot[3])
{
    for (int axis = 0; axis < 3; ++axis) foot[axis] = candidate[axis];
    int occupied = *rider;
    *rider = -1;
    if (occupied < -1 || occupied >= BFM_MINE_WHEEL_DECKS ||
        (occupied >= 0 && !last_supported)) return BFM_MINE_BASKET_MISS;

    /* An occupied deck gets the only test for this field, even if it left
     * the active set. On a later field the player can board a new one. */
    int first = occupied >= 0 ? occupied : 0;
    int end = occupied >= 0 ? occupied + 1 : BFM_MINE_WHEEL_DECKS;
    for (int index = first; index < end; ++index) {
        const BfmForestGuardBox *box;
        const BfmForestTransform *body = bfm_village_mine_wheels_deck(wheels, index, &box);
        if (!body) continue;
        BfmMineBasketContact result = bfm_mine_basket_probe(box, body, previous, candidate,
            occupied >= 0 ? last_supported : NULL, foot);
        if (result == BFM_MINE_BASKET_MISS) continue;
        if (result != BFM_MINE_BASKET_TOUCH) *rider = index;
        return result;
    }
    return BFM_MINE_BASKET_MISS;
}

void bfm_village_mine_decks_shadow(const BfmVillageMineWheels *wheels, const int16_t carried[3],
                                   BfmBedroomRoot *shadow)
{
    int16_t start[3] = {carried[0], (int16_t)(carried[1] - 16), carried[2]};
    int16_t end[3] = {carried[0], (int16_t)(carried[1] + 752), carried[2]};
    for (int index = 0; index < BFM_MINE_WHEEL_DECKS; ++index) {
        const BfmForestGuardBox *box;
        const BfmForestTransform *body = bfm_village_mine_wheels_deck(wheels, index, &box);
        int16_t center[3], contact[3], normal[3];
        if (body && bfm_forest_guard_contact_point(box, body, start, end, center, contact, normal) &&
            contact[1] < (shadow->shadow_visible ? shadow->shadow_xyz[1] : INT16_MAX)) {
            memcpy(shadow->shadow_xyz, contact, sizeof contact);
            memcpy(shadow->shadow_normal, normal, sizeof normal);
            shadow->shadow_visible = 1;
        }
    }
}

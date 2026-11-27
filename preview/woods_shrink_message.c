/* SPDX-License-Identifier: MIT */
#include "woods_shrink_message.h"
#include "../pc_port/forest_player_bytes.h"

BfmForestMessage *bfm_woods_ability_message_open(const uint8_t *scene, size_t scene_bytes,
                                                const uint8_t *main, size_t main_bytes,
                                                unsigned ability)
{
    /* The field's acquisition table points into the shared player payload.
     * Each script supplies its own page breaks and final dismissal. */
    size_t table;
    switch (ability) {
    case BFM_WOODS_MESSAGE_HOP: table = 0x66e80; break;
    case BFM_WOODS_MESSAGE_SATIATE:
    case BFM_WOODS_MESSAGE_SHRINK: table = 0x620b0; break;
    default: return NULL;
    }
    const size_t entry = table + 4 * ability;
    const uint32_t origin = UINT32_C(0x800cedf8);
    if (!scene || !main || scene_bytes < entry + 8) return NULL;
    uint32_t begin = u32le(scene + entry), end = u32le(scene + entry + 4);
    if (begin < origin || end <= begin || end - origin > main_bytes) return NULL;
    return bfm_field_message_borrow(main + (begin - origin), end - begin);
}

BfmForestMessage *bfm_woods_shrink_message_open(const uint8_t *scene, size_t scene_bytes,
                                               const uint8_t *main, size_t main_bytes)
{
    return bfm_woods_ability_message_open(scene, scene_bytes, main, main_bytes,
                                          BFM_WOODS_MESSAGE_SHRINK);
}

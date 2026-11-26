/* SPDX-License-Identifier: MIT */
#include "woods_shrink_message.h"
#include "../pc_port/forest_player_bytes.h"

BfmForestMessage *bfm_woods_shrink_message_open(const uint8_t *scene, size_t scene_bytes,
                                               const uint8_t *main, size_t main_bytes)
{
    /* The field's acquisition table points into the shared player payload.
     * The following entry bounds the whole announcement, including its two
     * page breaks and the button glyphs in its instructions. */
    enum { ACQUISITION_TABLE = 0x620b0, SHRINK_ABILITY = 13,
           ENTRY = ACQUISITION_TABLE + 4 * SHRINK_ABILITY };
    const uint32_t origin = UINT32_C(0x800cedf8);
    if (!scene || !main || scene_bytes < ENTRY + 8) return NULL;
    uint32_t begin = u32le(scene + ENTRY), end = u32le(scene + ENTRY + 4);
    if (begin < origin || end <= begin || end - origin > main_bytes) return NULL;
    return bfm_field_message_borrow(main + (begin - origin), end - begin);
}

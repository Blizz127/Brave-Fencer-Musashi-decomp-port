/* SPDX-License-Identifier: MIT */
#include "village_earth_portrait.h"
#include "dialogue_contour.h"

#include <string.h>

BfmVillageEarthPortrait *bfm_village_earth_portrait_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024])
{
    enum { UPPER_INSET = 0x5da74, TAIL_LIMITS = 6 };
    if (!disc || !scene || !tiles ||
        bytes < UPPER_INSET + 4 * BFM_DIALOGUE_CORNERS + 2 * TAIL_LIMITS)
        return NULL;

    /* The village keeps a single resident face in the window upload slot.
     * Decode Jon and Leno into adjacent slots of our private art in one
     * pass. Each face has eight 4bpp words on each of its forty rows. */
    uint16_t previous_faces[40][16], previous_palettes[2][16];
    for (unsigned y = 0; y < 40; ++y) {
        memcpy(previous_faces[y], &tiles[400 + y][456], sizeof previous_faces[y]);
        memcpy(&tiles[400 + y][456], &tiles[464 + y][696], 8 * sizeof(uint16_t));
        memcpy(&tiles[400 + y][464], &tiles[384 + y][648], 8 * sizeof(uint16_t));
    }
    memcpy(previous_palettes[0], &tiles[457][352], sizeof previous_palettes[0]);
    memcpy(previous_palettes[1], &tiles[458][352], sizeof previous_palettes[1]);
    memcpy(&tiles[457][352], &tiles[509][688], sizeof previous_palettes[0]);
    memcpy(&tiles[458][352], &tiles[504][656], sizeof previous_palettes[1]);

    BfmVillageEarthPortrait *art = bfm_castle_message_art_load(disc, scene, bytes,
                                                             tiles, UPPER_INSET);
    for (unsigned y = 0; y < 40; ++y)
        memcpy(&tiles[400 + y][456], previous_faces[y], sizeof previous_faces[y]);
    memcpy(&tiles[457][352], previous_palettes[0], sizeof previous_palettes[0]);
    memcpy(&tiles[458][352], previous_palettes[1], sizeof previous_palettes[1]);
    return art;
}

void bfm_village_earth_portrait_free(BfmVillageEarthPortrait *portraits)
{
    bfm_forest_message_art_free(portraits);
}

int bfm_village_earth_portrait_draw(const BfmVillageEarthPortrait *portraits,
    int current_selector, const BfmForestMessageFrame *frame, int16_t offset,
    int right, uint32_t pixels[240][320])
{
    if (!frame || frame->portrait > 2 || (current_selector != 24 && current_selector != 2))
        return 0;
    if (current_selector == 2 && frame->portrait == 2) {
        BfmForestMessageFrame bark = *frame;
        bark.portrait = 3;
        return bfm_forest_message_art_draw_placed(portraits, &bark, offset, right, pixels);
    }
    return bfm_forest_message_art_draw_placed(portraits, frame, offset, right, pixels);
}

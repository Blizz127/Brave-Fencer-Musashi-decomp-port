/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MESSAGE_ART_H
#define BFM_FOREST_MESSAGE_ART_H

#include "forest_message.h"
#include "forest_terrain.h"

typedef struct BfmForestMessageArt BfmForestMessageArt;

/* SC01/77 supplies the inset offsets and uploaded palettes. MAIN/10 owns the
 * font and contours. The returned object owns its decoded graphics. */
BfmForestMessageArt *bfm_forest_message_art_load(const MusashiDiscMedia *disc,
                                                const uint8_t *scene, size_t bytes,
                                                const BfmForestTerrain *terrain);
/* The room owns its contour inset and portrait already present in mounted VRAM. */
BfmForestMessageArt *bfm_bedroom_message_art_load(const MusashiDiscMedia *disc,
                                                  const uint8_t *scene, size_t bytes,
                                                  const uint16_t tiles[512][1024]);
BfmForestMessageArt *bfm_castle_message_art_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                                  size_t bytes, const uint16_t tiles[512][1024], size_t inset);
/* Cache the sign and Soldier Lardwick window art from the mounted SC03/29 room. */
BfmForestMessageArt *bfm_southern_woods_message_art_load(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint16_t tiles[512][1024]);
/* SC03/31 contains portraits for the gate party and Knitter Lunchetta. */
BfmForestMessageArt *bfm_valley_message_art_load(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint16_t tiles[512][1024]);
BfmForestMessageArt *bfm_mine_lake_message_art_load(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint16_t tiles[512][1024]);
/* Refresh dialogue slot two after a field script uploads the next speaker. */
void bfm_field_message_art_refresh_speaker(BfmForestMessageArt *art, const uint16_t tiles[512][1024]);
void bfm_forest_message_art_free(BfmForestMessageArt *art);
void bfm_forest_message_art_draw(const BfmForestMessageArt *art,
                                 const BfmForestMessageFrame *frame,
                                 uint32_t pixels[240][320]);
void bfm_bedroom_message_art_draw(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                  int speaker_x, uint32_t pixels[240][320]);
/* The active room command selects the side and positions the tail in screen space. */
void bfm_bedroom_message_art_draw_side(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                       int speaker_x, int portrait_on_right, uint32_t pixels[240][320]);
/* Draw a staged speaker's signed tail offset and side against the mounted
 * field's bubble rim. Returns zero if the placement table is unavailable. */
int bfm_forest_message_art_draw_placed(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                       int16_t offset, int on_right, uint32_t pixels[240][320]);
int bfm_library_question_draw(const BfmForestMessageArt *art, const uint8_t *box, const uint8_t *portrait,
                              const uint16_t tiles[512][1024], const uint8_t *text, size_t bytes,
                              unsigned update, uint32_t pixels[240][320]);
int bfm_field_menu_hint_draw(const BfmForestMessageArt *art, const uint8_t *box,
                             const uint16_t tiles[512][1024], const uint8_t *text, size_t bytes,
                             unsigned update, uint32_t pixels[240][320]);
/* The field item menu grows its reflected help contour over several fields.
 * Text appears once the authored window reaches its full fourteen columns. */
int bfm_field_menu_hint_draw_sized(const BfmForestMessageArt *art, const uint8_t *box,
    unsigned columns, unsigned lines, const uint8_t *text, size_t bytes,
    unsigned update, uint32_t pixels[240][320]);
#endif

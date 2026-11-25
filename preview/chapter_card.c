/* SPDX-License-Identifier: MIT */
#include "chapter_card.h"
#include "scene_tiles.h"
#include <stdlib.h>

enum { WIDTH = 320, HEIGHT = 240, CARD_W = 256, CARD_H = 144 };
struct BfmChapterCard { uint16_t color[CARD_H][CARD_W]; };

BfmChapterCard *bfm_chapter_card_load(const MusashiDiscMedia *disc)
{
    uint16_t (*tiles)[1024] = calloc(512, sizeof(*tiles));
    if (!tiles) return NULL;
    BfmChapterCard *card = NULL;
    if (bfm_scene_read_tiles(disc, "SC01.CD", 76, tiles)) {
        card = malloc(sizeof(*card));
        if (card) {
            for (unsigned y = 0; y < CARD_H; ++y) {
                for (unsigned x = 0; x < CARD_W; ++x) {
                    unsigned pair = tiles[y][320 + x / 2];
                    unsigned index = pair >> (8 * (x % 2)) & 255u;
                    card->color[y][x] = tiles[480][index];
                }
            }
        }
    }
    free(tiles);
    return card;
}

void bfm_chapter_card_free(BfmChapterCard *card) { free(card); }

static unsigned row_shade(unsigned elapsed, unsigned y)
{
    if (elapsed < 108) return 31;
    if (elapsed < 140) return 139u - (elapsed & ~1u);
    if (elapsed < 356) return 0;
    /* Each pair of eight-line strips recedes from the outside inward. */
    unsigned near_edge = y < CARD_H / 2 ? y : CARD_H - 1 - y;
    unsigned wave = 1 + (elapsed - 356) / 4;
    unsigned band = near_edge / 8;
    return wave > band ? wave - band : 0;
}

int bfm_chapter_card_draw(const BfmChapterCard *card, unsigned elapsed, unsigned source_member, uint32_t *pixels)
{
    if (!card || !pixels) return -1;
    for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) pixels[i] = UINT32_C(0xff000000);
    if (elapsed >= 638) return 0;
    unsigned card_field = elapsed;
    if (source_member == 0 && elapsed < 140) card_field += 4;
    if (card_field < 108 || elapsed >= (source_member ? 506u : 504u)) return 1;
    for (unsigned y = 0; y < CARD_H; ++y) {
        unsigned shade = row_shade(card_field, y);
        for (unsigned x = 0; x < CARD_W; ++x) {
            unsigned word = card->color[y][x];
            uint32_t rgb = UINT32_C(0xff000000);
            for (unsigned k = 0; k < 3; ++k) {
                unsigned c = (word >> (5 * k)) & 31u;
                c = c > shade ? (c - shade) * 255u / 31u : 0;
                rgb |= c << (16 - 8 * k);
            }
            pixels[(48 + y) * WIDTH + 32 + x] = rgb;
        }
    }
    return 1;
}

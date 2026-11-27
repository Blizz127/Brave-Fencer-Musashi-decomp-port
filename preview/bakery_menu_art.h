/* SPDX-License-Identifier: MIT */
#ifndef BFM_BAKERY_MENU_ART_H
#define BFM_BAKERY_MENU_ART_H

#include "save_menu.h"

enum { BFM_BAKERY_VISIBLE_ROWS = 6, BFM_BAKERY_BAG_SLOTS = 12 };
typedef enum { BFM_BAKERY_SERVICES, BFM_BAKERY_GOODS, BFM_BAKERY_ITEMS } BfmBakeryPane;
typedef struct BfmBakeryMenuArt BfmBakeryMenuArt;
typedef struct { const uint8_t *name; size_t name_bytes; } BfmBakeryName;
typedef struct {
    BfmBakeryPane active;
    unsigned service, good, item;
    /* Catalogue offset and total number of goods; age advances on the shop clock. */
    unsigned first_good, goods_count, age;
    /* Names are the mounted two-byte labels, excluding their NUL. A NULL
     * item preserves that empty slot. The bag displays two columns of six. */
    BfmBakeryName goods[BFM_BAKERY_VISIBLE_ROWS], items[BFM_BAKERY_BAG_SLOTS];
    unsigned drans[BFM_BAKERY_VISIBLE_ROWS];
} BfmBakeryMenuView;

BfmBakeryMenuArt *bfm_bakery_menu_art_load(const MusashiDiscMedia *disc,
                                          const uint8_t *village, size_t bytes);
void bfm_bakery_menu_art_free(BfmBakeryMenuArt *menu);
/* Draw over a freshly rendered field. The caller draws Jam's dialogue and HUD. */
int bfm_bakery_menu_draw(const BfmBakeryMenuArt *menu, const BfmSaveArt *art,
                         const BfmBakeryMenuView *view, uint32_t pixels[240][320]);

/* Interface to the field overlay painter, which shares the inn's font atlas. */
typedef struct { int x, y, w, h; } BfmShopRect;
typedef struct { BfmShopRect rect; unsigned u, v; } BfmShopBorder;
typedef struct { BfmShopRect rect; uint8_t top[3], bottom[3]; } BfmShopFill;
typedef struct { uint16_t uv[12]; unsigned count; } BfmShopLabel;
typedef struct {
    BfmShopBorder borders[8];
    BfmShopFill fills[2];
    BfmShopLabel title;
    int title_x, title_y;
    int row_x[BFM_BAKERY_BAG_SLOTS], row_y[BFM_BAKERY_BAG_SLOTS];
} BfmShopListLayout;

void bfm_save_art_shop_list(const BfmSaveArt *art, uint32_t pixels[240][320],
                            const BfmShopListLayout *box, BfmBakeryPane pane, const BfmBakeryMenuView *view,
                            unsigned cursor, const BfmShopLabel *names,
                            const unsigned *prices, unsigned count);
#endif

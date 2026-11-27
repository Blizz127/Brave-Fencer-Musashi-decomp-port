/* SPDX-License-Identifier: MIT */
#include "bakery_service.h"

enum {
    PAD_UP = 0x10, PAD_RIGHT = 0x20, PAD_DOWN = 0x40, PAD_LEFT = 0x80,
    PAD_BACK = 0x1000, PAD_ACCEPT = 0x4000
};
enum {
    MOVE_SOUND = 1114, FULL_SOUND = 1119, OPEN_SOUND = 1135, REJECT_SOUND = 1138,
    BACK_SOUND = 1139, QUIT_SOUND = 1140, TRADE_SOUND = 1518
};

int bfm_bakery_service_begin(BfmBakeryService *s, const uint8_t *scene, size_t scene_bytes,
                             const uint8_t *main, size_t main_bytes, const BfmSaveRecord *record)
{
    if (!s) return 0;
    BfmBakeryService fresh = {.pane = BFM_BAKERY_SERVICES, .caption = BFM_BAKERY_CAPTION_OFFER,
                              .main = main, .main_bytes = main_bytes};
    if (!bfm_bakery_stock(&fresh.stock, scene, scene_bytes, main, main_bytes, record)) return 0;
    *s = fresh;
    return 1;
}

unsigned bfm_bakery_service_item(const BfmBakeryService *s, const BfmSaveRecord *record)
{
    if (!s || !record) return 0;
    if (s->pane == BFM_BAKERY_GOODS && s->good < s->stock.count)
        return s->stock.goods[s->good].item;
    if (s->pane == BFM_BAKERY_ITEMS && s->item < BFM_BAKERY_BAG_SLOTS)
        return record->variables[47 + s->item];
    return 0;
}

static int selection_caption(BfmBakeryService *s, const BfmSaveRecord *record)
{
    s->quote_drans = 0;
    if (s->pane == BFM_BAKERY_SERVICES) {
        s->caption = BFM_BAKERY_CAPTION_OFFER;
    } else if (s->pane == BFM_BAKERY_GOODS) {
        s->caption = BFM_BAKERY_CAPTION_ITEM;
    } else {
        unsigned item = bfm_bakery_service_item(s, record);
        s->caption = BFM_BAKERY_CAPTION_EMPTY;
        if (item) {
            BfmBakeryGood good;
            if (!bfm_bakery_item(s->main, s->main_bytes, item, &good)) return 0;
            s->quote_drans = good.sell_drans;
            s->caption = good.sell_drans ? BFM_BAKERY_CAPTION_SALE_OFFER : BFM_BAKERY_CAPTION_REFUSED;
        }
    }
    return 1;
}

uint16_t bfm_bakery_service_pad(BfmBakeryService *s, uint16_t pressed, uint16_t held)
{
    unsigned directions = PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT;
    if (!held) {
        s->repeat_wait = 12;
    } else if (held != s->last_held) {
        pressed |= (held & ~s->last_held) & directions;
        s->repeat_wait = 8;
    } else if (s->repeat_wait && !--s->repeat_wait) {
        pressed |= held & directions;
        s->repeat_wait = 3;
    }
    s->last_held = held;
    return pressed;
}

static void navigate(BfmBakeryService *s, uint16_t pressed)
{
    if (s->pane == BFM_BAKERY_GOODS) {
        unsigned row = s->good;
        if ((pressed & PAD_UP) && row) --row;
        if ((pressed & PAD_DOWN) && row + 1 < s->stock.count) ++row;
        if (row != s->good) {
            s->good = row;
            if (row < s->first_good) s->first_good = row;
            if (row >= s->first_good + BFM_BAKERY_VISIBLE_ROWS)
                s->first_good = row + 1 - BFM_BAKERY_VISIBLE_ROWS;
            s->sound = MOVE_SOUND;
        }
    } else if (s->pane == BFM_BAKERY_SERVICES) {
        unsigned row = s->service;
        if (pressed & PAD_UP) row = (row + 2) % 3;
        if (pressed & PAD_DOWN) row = (row + 1) % 3;
        if (row != s->service) {
            s->service = row;
            s->sound = MOVE_SOUND;
        }
    } else {
        unsigned row = s->item % BFM_BAKERY_VISIBLE_ROWS;
        unsigned column = s->item / BFM_BAKERY_VISIBLE_ROWS;
        if (pressed & PAD_UP) row = (row + BFM_BAKERY_VISIBLE_ROWS - 1) % BFM_BAKERY_VISIBLE_ROWS;
        if (pressed & PAD_DOWN) row = (row + 1) % BFM_BAKERY_VISIBLE_ROWS;
        if (pressed & PAD_LEFT) column ^= 1;
        if (pressed & PAD_RIGHT) column ^= 1;
        unsigned next = column * BFM_BAKERY_VISIBLE_ROWS + row;
        if (s->item != next) {
            s->item = next;
            s->sound = MOVE_SOUND;
        }
    }
}

static int buy_selection(BfmBakeryService *s, BfmSaveRecord *record, BfmGameplayRun *run)
{
    switch (bfm_bakery_buy(&s->stock, s->good, record, run)) {
    case BFM_BAKERY_BOUGHT:
        s->caption = BFM_BAKERY_CAPTION_BOUGHT;
        s->sound = TRADE_SOUND;
        return 1;
    case BFM_BAKERY_BAG_FULL:
        s->caption = BFM_BAKERY_CAPTION_FULL;
        s->sound = FULL_SOUND;
        return 1;
    case BFM_BAKERY_NO_FUNDS:
        s->caption = BFM_BAKERY_CAPTION_POOR;
        s->sound = REJECT_SOUND;
        s->sound_arg = 3;
        return 1;
    default:
        return 0;
    }
}

static int sell_selection(BfmBakeryService *s, BfmSaveRecord *record, BfmGameplayRun *run)
{
    switch (bfm_bakery_sell(s->main, s->main_bytes, s->item, record, run)) {
    case BFM_BAKERY_SOLD:
        s->caption = BFM_BAKERY_CAPTION_SOLD;
        s->sound = TRADE_SOUND;
        return 1;
    case BFM_BAKERY_NOT_WANTED:
        s->caption = BFM_BAKERY_CAPTION_REFUSED;
        s->sound = REJECT_SOUND;
        return 1;
    case BFM_BAKERY_SELL_EMPTY:
        s->caption = BFM_BAKERY_CAPTION_EMPTY;
        s->sound = REJECT_SOUND;
        return 1;
    default:
        return 0;
    }
}

BfmBakeryServiceResult bfm_bakery_service_step(BfmBakeryService *s, uint16_t pressed,
                                              BfmSaveRecord *record, BfmGameplayRun *run)
{
    if (!s || !record || !s->stock.count) return BFM_BAKERY_SERVICE_ERROR;
    s->sound = s->sound_arg = 0;
    if (!pressed) return BFM_BAKERY_SERVICE_ACTIVE;
    if (pressed & PAD_ACCEPT) {
        if (s->pane == BFM_BAKERY_GOODS)
            return buy_selection(s, record, run) ? BFM_BAKERY_SERVICE_ACTIVE : BFM_BAKERY_SERVICE_ERROR;
        if (s->pane == BFM_BAKERY_ITEMS)
            return sell_selection(s, record, run) ? BFM_BAKERY_SERVICE_ACTIVE : BFM_BAKERY_SERVICE_ERROR;
        if (s->service == 2) {
            s->sound = QUIT_SOUND;
            return BFM_BAKERY_SERVICE_QUIT;
        }
        s->pane = s->service == 0 ? BFM_BAKERY_GOODS : BFM_BAKERY_ITEMS;
        s->sound = OPEN_SOUND;
    } else if (pressed & PAD_BACK) {
        if (s->pane == BFM_BAKERY_SERVICES) {
            s->sound = QUIT_SOUND;
            return BFM_BAKERY_SERVICE_QUIT;
        }
        s->pane = BFM_BAKERY_SERVICES;
        s->sound = BACK_SOUND;
    } else {
        navigate(s, pressed);
    }
    return selection_caption(s, record) ? BFM_BAKERY_SERVICE_ACTIVE : BFM_BAKERY_SERVICE_ERROR;
}

int bfm_bakery_service_view(const BfmBakeryService *s, const BfmSaveRecord *record,
                            BfmBakeryMenuView *view)
{
    if (!s || !record || !view || !s->stock.count) return 0;
    BfmBakeryMenuView next = {.active = s->pane, .service = s->service,
                              .good = s->good - s->first_good, .item = s->item,
                              .first_good = s->first_good, .goods_count = s->stock.count};
    for (unsigned i = 0; i < BFM_BAKERY_VISIBLE_ROWS && s->first_good + i < s->stock.count; ++i) {
        const BfmBakeryGood *good = &s->stock.goods[s->first_good + i];
        next.goods[i] = (BfmBakeryName){good->name, good->name_bytes};
        next.drans[i] = good->drans;
    }
    for (unsigned i = 0; i < BFM_BAKERY_BAG_SLOTS; ++i) {
        unsigned item = record->variables[47 + i];
        if (!item) continue;
        BfmBakeryGood held;
        if (!bfm_bakery_item(s->main, s->main_bytes, item, &held)) return 0;
        next.items[i] = (BfmBakeryName){held.name, held.name_bytes};
    }
    *view = next;
    return 1;
}

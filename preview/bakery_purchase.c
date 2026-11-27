/* SPDX-License-Identifier: MIT */
#include "bakery_purchase.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

enum {
    CATALOGUE = 0x3fff0, CATALOGUE_ROWS = 125, ITEM_BYTES = 12,
    BAKERY_STOCK = 0x801c16b4u - 0x80128158u
};

int bfm_bakery_item(const uint8_t *main, size_t main_bytes, unsigned item, BfmBakeryGood *out)
{
    if (!main || !out || !item || item >= CATALOGUE_ROWS) return 0;
    size_t row = CATALOGUE + item * ITEM_BYTES;
    if (main_bytes < row + ITEM_BYTES) return 0;
    uint32_t address = u32le(main + row);
    const uint32_t origin = UINT32_C(0x800cedf8);
    if (address < origin || address - origin >= main_bytes) return 0;
    size_t start = address - origin;
    const uint8_t *name = main + start;
    const uint8_t *end = memchr(name, 0, main_bytes - start);
    if (!end || end == name || (end - name) % 2) return 0;
    int resale = s16le(main + row + 6);
    unsigned offer = resale > 0 && main[row + 10] == 1 ? (unsigned)resale * 10u : 0;
    *out = (BfmBakeryGood){.item = item, .drans = 10u * u16le(main + row + 4),
                           .sell_drans = offer, .name = name, .name_bytes = (size_t)(end - name)};
    return 1;
}

int bfm_bakery_stock(BfmBakeryPurchase *shop, const uint8_t *scene, size_t scene_bytes,
                     const uint8_t *main, size_t main_bytes, const BfmSaveRecord *state)
{
    if (!shop || !scene || !state) return 0;
    BfmBakeryPurchase stock = {.count = u32le(state->progress + 8) < 600 ? 5u : 7u};
    if (scene_bytes < BAKERY_STOCK + stock.count) return 0;
    unsigned markdown = s16le(state->progress + 0x1a) >= 18 * 60 ? 30u : 0u;
    for (unsigned row = 0; row < stock.count; ++row) {
        BfmBakeryGood *good = &stock.goods[row];
        if (!bfm_bakery_item(main, main_bytes, scene[BAKERY_STOCK + row], good) ||
            good->drans <= markdown) return 0;
        good->drans -= markdown;
    }
    *shop = stock;
    return 1;
}

static void set_wallet(BfmSaveRecord *state, BfmGameplayRun *run, uint32_t units)
{
    for (unsigned i = 0; i < 4; ++i) state->progress[0x14 + i] = (uint8_t)(units >> (8 * i));
    if (run) memcpy(run->player.record + 0x14, state->progress + 0x14, 4);
}

BfmBakerySellResult bfm_bakery_sell(const uint8_t *main, size_t main_bytes, unsigned slot,
                                    BfmSaveRecord *state, BfmGameplayRun *run)
{
    if (!state || slot >= 12) return BFM_BAKERY_SELL_ERROR;
    unsigned item = state->variables[47 + slot];
    if (!item) return BFM_BAKERY_SELL_EMPTY;
    BfmBakeryGood good;
    if (!bfm_bakery_item(main, main_bytes, item, &good)) return BFM_BAKERY_SELL_ERROR;
    if (!good.sell_drans) return BFM_BAKERY_NOT_WANTED;
    unsigned credit = good.sell_drans / 10u;
    uint32_t wallet = u32le(state->progress + 0x14);
    set_wallet(state, run, wallet > 99999u - credit ? 99999u : wallet + credit);
    state->variables[47 + slot] = 0;
    if (run) run->banks.variables[47 + slot] = 0;
    return BFM_BAKERY_SOLD;
}

BfmBakeryBuyResult bfm_bakery_buy(const BfmBakeryPurchase *shop, unsigned selected,
                                  BfmSaveRecord *state, BfmGameplayRun *run)
{
    if (!shop || !state || selected >= shop->count || selected >= 7) return BFM_BAKERY_BUY_ERROR;
    const BfmBakeryGood *good = &shop->goods[selected];
    if (!good->drans || good->drans % 10u || !good->item || good->item >= CATALOGUE_ROWS)
        return BFM_BAKERY_BUY_ERROR;

    int slot = bfm_save_inventory_space(state);
    if (slot < 0) return BFM_BAKERY_BAG_FULL;
    uint32_t wallet = u32le(state->progress + 0x14);
    unsigned charge = good->drans / 10u;
    if (wallet < charge) return BFM_BAKERY_NO_FUNDS;
    if (!bfm_save_add_item(state, good->item)) return BFM_BAKERY_BUY_ERROR;
    set_wallet(state, run, wallet - charge);
    if (run) {
        run->banks.variables[47 + slot] = state->variables[47 + slot];
        memcpy(run->banks.variables + 20 + 2 * slot, state->variables + 20 + 2 * slot, 2);
    }
    return BFM_BAKERY_BOUGHT;
}

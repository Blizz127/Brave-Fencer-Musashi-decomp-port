/* SPDX-License-Identifier: MIT */
#include "woods_king_reward.h"
#include "bedroom_score.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_woods_king_reward_load(BfmWoodsKingReward *reward, const MusashiDiscMedia *disc,
                               const uint8_t *main, size_t main_bytes)
{
    const size_t offer = UINT32_C(0x8010f468) + 9 * 8 - UINT32_C(0x800cedf8);
    if (!reward || !disc || !main || main_bytes < offer + 7 ||
        !bfm_bedroom_resident_data(disc, UINT32_C(0x800638f8),
                                   reward->ranks[0], sizeof reward->ranks)) return 0;
    reward->figure_price = u16le(main + offer + 4);
    reward->figure_trend = main[offer + 6];
    return 1;
}

static void write_word(uint8_t *out, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        out[i] = (uint8_t)(value >> (8 * i));
}

unsigned bfm_woods_king_reward_apply(const BfmWoodsKingReward *reward, unsigned experience,
                                     BfmSaveRecord *save)
{
    if (!reward || !save) return 0;
    /* The ninth figure becomes available for purchase. Preserve its current
     * price and ownership on a save where the shop already carries it. */
    if (!(save->variables[107] & 0x80u)) {
        save->variables[107] = reward->figure_trend | 0x80u;
        save->figure_prices[16] = (uint8_t)reward->figure_price;
        save->figure_prices[17] = (uint8_t)(reward->figure_price >> 8);
    }
    uint32_t story = u32le(save->progress + 8);
    uint32_t rank = u32le(save->progress + 0x6c);
    if (story < 10 || rank >= 29) return 0;
    unsigned threshold = u16le(reward->ranks[rank]);
    if (!threshold) return 0;
    uint32_t earned = u32le(save->progress + 0x70) + experience;
    if (earned < threshold) {
        write_word(save->progress + 0x70, earned);
        return 0;
    }
    unsigned cap = story < 200 ? 8 : story < 600 ? 16 : story < 900 ? 22 :
                   story < 1200 ? 27 : 99;
    if (rank + 1 >= cap) {
        write_word(save->progress + 0x70, threshold);
        return 0;
    }
    write_word(save->progress + 0x6c, rank + 1);
    write_word(save->progress + 0x70, 0);
    unsigned new_body = u16le(reward->ranks[rank + 1] + 2);
    unsigned old_body = u16le(reward->ranks[rank] + 2);
    return new_body >= old_body ? new_body - old_body : 0;
}

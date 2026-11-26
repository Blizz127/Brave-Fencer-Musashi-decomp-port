/* SPDX-License-Identifier: MIT */
#include "forest_award.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

enum { SECTOR = 2048 };

int bfm_forest_award_begin(BfmForestAward *award, const MusashiDiscMedia *disc,
    const BfmForestCast *cast, const BfmForestTether *tether, BfmForestStatus *status)
{
    if (!award || !cast || !tether || !status || award->phase != BFM_AWARD_IDLE ||
        !disc || tether->phase != BFM_TETHER_EARNED || tether->ability != 8 ||
        cast->award.hold_after != 1 || cast->award.count < 2) return 0;
    ForestPlayerArchive player;
    uint8_t resource[8], directory[SECTOR];
    size_t total;
    /* The Gunshot selection in the resident player loader uses row two.
     * Its first word names the MAIN.CD member and its second the load slot. */
    if (!forest_player_archive_open(disc, &player) ||
        !forest_player_archive_read(&player, UINT32_C(0x800d3774), resource, sizeof resource) ||
        u32le(resource + 4) != 5 || !musashi_disc_media_read_root_range(disc, "MAIN.CD",
            0, directory, sizeof directory, &total)) return 0;
    uint32_t member = u32le(resource);
    if (member >= u32le(directory) || member >= (SECTOR - 8u) / 8u) return 0;
    const uint8_t *entry = directory + 8 + 8 * member;
    uint32_t sector = u32le(entry);
    size_t bytes = u32le(entry + 4);
    if (!sector || sector > total / SECTOR || bytes < SECTOR ||
        bytes > sizeof award->package || bytes % SECTOR) return 0;
    size_t offset = (size_t)sector * SECTOR;
    if (bytes > total - offset) return 0;
    award->disc = disc;
    award->offset = offset;
    award->bytes = bytes;
    award->loaded = award->key = 0;
    award->pose = cast->award.keys[0].pose;
    award->visible = cast->award.keys[0].visible;
    award->phase = BFM_AWARD_CATCH;
    status->record[0x49] = 0;
    return 1;
}

static int equipped_resource_ready(const BfmForestAward *award)
{
    unsigned seen = 0;
    for (size_t at = 0; at < award->bytes; ) {
        const uint8_t *section = award->package + at;
        if (award->bytes - at < SECTOR || memcmp(section, "PAC", 4) || section[4] > 3)
            return 0;
        unsigned kind = 1u << section[4];
        size_t bytes = u32le(section + 12);
        if ((seen & kind) || bytes < SECTOR || bytes > award->bytes - at) return 0;
        seen |= kind;
        at += ((bytes - 1) / SECTOR + 1) * SECTOR;
    }
    return seen == 15;
}

BfmForestAwardPhase bfm_forest_award_tick(BfmForestAward *award, const BfmForestCast *cast,
    BfmForestTether *tether, BfmForestStatus *status, int closed)
{
    if (!award || !cast || !tether || !status) return BFM_AWARD_ERROR;
    if (award->phase != BFM_AWARD_IDLE && award->phase != BFM_AWARD_COMPLETE &&
        tether->phase != BFM_TETHER_EARNED) return award->phase = BFM_AWARD_ERROR;
    switch (award->phase) {
    case BFM_AWARD_CATCH:
        return award->phase = BFM_AWARD_LOADING;
    case BFM_AWARD_LOADING:
        if (award->loaded == award->bytes)
            return award->phase = equipped_resource_ready(award) ? BFM_AWARD_OPEN : BFM_AWARD_ERROR;
        {
            size_t total;
            if (!musashi_disc_media_read_root_range(award->disc, "MAIN.CD",
                award->offset + award->loaded, award->package + award->loaded, SECTOR, &total))
                return award->phase = BFM_AWARD_ERROR;
            award->loaded += SECTOR;
        }
        break;
    case BFM_AWARD_OPEN:
    case BFM_AWARD_WAIT:
        return award->phase = closed ? BFM_AWARD_CLEAR : BFM_AWARD_WAIT;
    case BFM_AWARD_CLEAR:
        return award->phase = BFM_AWARD_RELEASE;
    case BFM_AWARD_RELEASE:
        if (++award->key == cast->award.count)
            return award->phase = bfm_forest_tether_commit(tether, status) ?
                BFM_AWARD_COMPLETE : BFM_AWARD_ERROR;
        award->pose = cast->award.keys[award->key].pose;
        award->visible = cast->award.keys[award->key].visible;
        break;
    default: break;
    }
    return award->phase;
}

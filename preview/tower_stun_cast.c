/* SPDX-License-Identifier: MIT */
#include "tower_stun_cast.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { SECTOR = 2048, MAX_KEYS = 32 };

typedef struct {
    BfmForestPose body;
    uint32_t pose;
    int8_t blade[12];
    unsigned window;
} StunKey;

struct BfmTowerStunCast {
    StunKey keys[MAX_KEYS];
    uint32_t visible;
    unsigned count, next;
    int8_t tips[2][3];
};

/* Resolve the ability's package through the mounted player load list.
 * An ability member can contain additional PAC groups after the code and
 * animation section. The first uncompressed type-one section owns the clip. */
static uint8_t *load_section(const MusashiDiscMedia *disc, const ForestPlayerArchive *player,
                             size_t *payload_size)
{
    uint8_t row[8], directory[SECTOR], header[16];
    size_t total;
    if (!forest_player_archive_read(player, UINT32_C(0x800d3764), row, sizeof row) ||
        u32le(row + 4) != 7 ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", 0, directory, sizeof directory, &total))
        return NULL;
    uint32_t member = u32le(row), count = u32le(directory);
    if (member >= count || count > (SECTOR - 8u) / 8u) return NULL;
    const uint8_t *entry = directory + 8 + member * 8;
    size_t sector = u32le(entry), remaining = u32le(entry + 4);
    if (!sector || sector > total / SECTOR) return NULL;
    size_t cursor = sector * SECTOR;
    if (remaining > total - cursor || remaining % SECTOR) return NULL;
    while (remaining >= SECTOR) {
        if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", cursor, header, sizeof header, &total) ||
            memcmp(header, "PAC", 4)) return NULL;
        size_t length = u32le(header + 12);
        if (length < SECTOR || length > remaining) return NULL;
        size_t sectors = (length - 1u) / SECTOR + 1u;
        if (sectors > remaining / SECTOR) return NULL;
        if (header[4] == 1 && header[5] == 0) {
            size_t bytes = length - SECTOR;
            /* The ability's slot ends where the resident player block starts. */
            if (!bytes || bytes > UINT32_C(0x800cedf8) - UINT32_C(0x800cae08)) return NULL;
            uint8_t *section = malloc(bytes);
            if (!section) return NULL;
            if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", cursor + SECTOR,
                                                    section, bytes, &total)) {
                free(section);
                return NULL;
            }
            *payload_size = bytes;
            return section;
        }
        remaining -= sectors * SECTOR;
        cursor += sectors * SECTOR;
    }
    return NULL;
}

static const uint8_t *section_at(const uint8_t *section, size_t size, uint32_t address, size_t bytes)
{
    const uint32_t origin = UINT32_C(0x800cae08);
    if (address < origin || (address & 3)) return NULL;
    size_t offset = address - origin;
    return offset <= size && bytes <= size - offset ? section + offset : NULL;
}

static int load_motion(BfmTowerStunCast *cast, const ForestPlayerArchive *player,
                       const uint8_t *section, size_t size)
{
    const uint32_t script = UINT32_C(0x800cb830);
    const uint8_t *setup = section_at(section, size, script, 12);
    uint8_t parts[4 * (MUSASHI_FOREST_PLAYER_PARTS + 1)];
    if (!setup || u32le(setup) != 4 || u32le(setup + 8) != 19 ||
        !forest_player_archive_read(player, u32le(setup + 4), parts, sizeof parts) ||
        u32le(parts + 4 * MUSASHI_FOREST_PLAYER_PARTS)) return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if ((u32le(parts + 4 * i) >> 24) != 0x80) return 0;
        cast->visible |= UINT32_C(1) << i;
    }
    /* Fusion is equipped when Stun is earned here. */
    cast->visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 24) | (UINT32_C(1) << 25));
    unsigned window = 0, edges = 0, charge_phase = 0;
    int scale = 4096;
    for (unsigned i = 3; i < 64; ++i) {
        const uint8_t *word = section_at(section, size, script + 4 * i, 4);
        if (!word) return 0;
        uint32_t value = u32le(word);
        if (!value) return cast->count > 0 && edges == 2 && !window && charge_phase == 2;
        if (value == 8) {
            if (++charge_phase > 2) return 0;
            continue;
        }
        if (value == 20) {
            window ^= 1u;
            ++edges;
            continue;
        }
        const uint8_t *packed = section_at(section, size, value, 256);
        if (!packed || cast->count >= MAX_KEYS) return 0;
        StunKey *key = &cast->keys[cast->count];
        if (!bfm_forest_pose_decode(packed + 16, 240, &key->body)) return 0;
        /* The charge markers enlarge the Fusion blade and the hand holding
         * it, then release them. The packed positions and hit rays stay fixed. */
        if (charge_phase) {
            scale += charge_phase == 1 ? 2048 : -2048;
            if (scale < 4096 || scale > 12288) return 0;
            const unsigned parts_scaled[2] = {8, 22};
            for (unsigned part = 0; part < 2; ++part) {
                BfmForestTransform *joint = &key->body.local[parts_scaled[part]];
                for (unsigned row = 0; row < 3; ++row)
                    for (unsigned col = 0; col < 3; ++col) {
                        int product = joint->rotation[row][col] * scale;
                        joint->rotation[row][col] = (int16_t)(product / 4096 - (product % 4096 < 0));
                    }
            }
        }
        key->pose = value + 16;
        memcpy(key->blade, packed, sizeof key->blade);
        key->window = window;
        ++cast->count;
    }
    return 0;
}

BfmTowerStunCast *bfm_tower_stun_cast_open(const MusashiDiscMedia *disc, const BfmForestField *field)
{
    ForestPlayerArchive player;
    if (!disc || !field || !forest_player_archive_open(disc, &player)) return NULL;
    const uint8_t *strike = bfm_forest_room_bytes(field, UINT32_C(0x8018cc38), 20);
    if (!strike || u32le(strike) != 15) return NULL;
    uint32_t points = u32le(strike + 4);
    if (points >> 24 != 0x81) return NULL;
    const uint8_t *tips = bfm_forest_room_bytes(field, points & UINT32_C(0x80ffffff), 6);
    if (!tips) return NULL;
    size_t size = 0;
    uint8_t *section = load_section(disc, &player, &size);
    if (!section) return NULL;
    BfmTowerStunCast *cast = calloc(1, sizeof *cast);
    if (cast && load_motion(cast, &player, section, size)) {
        memcpy(cast->tips, tips, sizeof cast->tips);
        cast->next = cast->count;
    } else {
        free(cast);
        cast = NULL;
    }
    free(section);
    return cast;
}

void bfm_tower_stun_cast_free(BfmTowerStunCast *cast)
{
    free(cast);
}

int bfm_tower_stun_cast_tick(BfmTowerStunCast *cast, int begin, const BfmForestTransform *root,
                             BfmTowerStunFrame *frame)
{
    if (!frame) return 0;
    *frame = (BfmTowerStunFrame){0};
    if (!cast || !root) return 0;
    if (cast->next == cast->count) {
        if (!begin) return 0;
        cast->next = 0;
    }
    const StunKey *key = &cast->keys[cast->next];
    frame->key = cast->next++;
    frame->body = &key->body;
    frame->pose = key->pose;
    frame->visible = cast->visible;
    memcpy(frame->blade, key->blade, sizeof frame->blade);
    if (key->window)
        frame->ray_count = (unsigned)bfm_forest_blade_rays(root, cast->tips[0], cast->tips[1],
                                                           frame->origin, frame->end);
    return 1;
}

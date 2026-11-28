/* SPDX-License-Identifier: MIT */
#include "village_mine_currents.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

enum { PROFILES = 3, BANDS = 5, VECTOR_BYTES = 8, CURRENT_SOUND = 0x738 };

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t need)
{
    const uint32_t base = UINT32_C(0x80128158);
    return scene && address >= base && need <= bytes && address - base <= bytes - need
        ? scene + (address - base) : NULL;
}

int bfm_mine_currents_load(BfmMineCurrents *currents, const uint8_t *scene, size_t bytes)
{
    if (!currents) return 0;
    const uint8_t *binding = mounted(scene, bytes, UINT32_C(0x80181d20), 4);
    if (!binding || u32le(binding) != UINT32_C(0x80185a50)) return 0;
    const uint8_t *room = mounted(scene, bytes, u32le(binding), 0xbc);
    if (!room) return 0;
    uint32_t begin = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= begin || (end - begin) % 20) return 0;
    const uint8_t *rows = mounted(scene, bytes, begin, end - begin);
    BfmMineCurrents next = {0};
    next.vectors = mounted(scene, bytes, UINT32_C(0x80185988), PROFILES * BANDS * VECTOR_BYTES);
    if (!rows || !next.vectors) return 0;
    unsigned n = 0;
    for (size_t i = 0; i < end - begin; i += 20) {
        const uint8_t *row = rows + i;
        unsigned kind = u16le(row + 6);
        if (!kind) break;
        if (kind != 484) continue;
        unsigned profile = u16le(row + 8);
        if (n == BFM_MINE_CURRENT_COUNT || profile >= PROFILES) return 0;
        next.source[n++] = (BfmMineCurrentSource){s16le(row), s16le(row + 4), profile};
    }
    if (n != BFM_MINE_CURRENT_COUNT) return 0;
    *currents = next;
    return 1;
}

int bfm_mine_currents_take(BfmMineCurrents *currents, int16_t vector[3])
{
    if (!currents || !vector || !currents->marked) return 0;
    memcpy(vector, currents->queued, sizeof currents->queued);
    currents->marked = 0;
    return 1;
}

static int source_band(const BfmMineCurrentSource *source, const int16_t foot[3])
{
    int16_t dx = (int16_t)(source->x - foot[0]), dz = (int16_t)(source->z - foot[2]);
    int along = source->profile == 1 ? dz : dx;
    int across = source->profile == 1 ? dx : dz;
    if (along < -432 || along > 432 || across < -96 || across > 96) return -1;
    return (along >= -352) + (along >= -288) + (along >= 288) + (along >= 352);
}

static BfmMineCurrentSound current_sound(const BfmMineCurrents *currents, unsigned occupied,
                                         const int16_t foot[3], int inside)
{
    const BfmMineCurrentSound full = {CURRENT_SOUND, 0x107f};
    if (inside) return full;
    int64_t nearest = INT64_MAX;
    for (unsigned i = 0; i < BFM_MINE_CURRENT_COUNT; ++i) {
        if (!(occupied & (1u << i))) continue;
        int64_t dx = currents->source[i].x - foot[0], dz = currents->source[i].z - foot[2];
        int64_t distance = dx * dx + dz * dz;
        if (distance < nearest) nearest = distance;
    }
    if (nearest <= 0x40000) return full;
    if (nearest >= 0x140000) return (BfmMineCurrentSound){4, CURRENT_SOUND};
    return (BfmMineCurrentSound){CURRENT_SOUND, 0x1000u | (unsigned)((0x240000 - nearest) >> 13)};
}

void bfm_mine_currents_visit(BfmMineCurrents *currents, unsigned source, unsigned occupied,
    const BfmVillageGround *ground, const int16_t foot[3], BfmMineCurrentSound *sound)
{
    if (!currents || !currents->vectors || !foot || source >= BFM_MINE_CURRENT_COUNT ||
        !(occupied & (1u << source))) return;
    const BfmMineCurrentSource *actor = &currents->source[source];
    int band = source_band(actor, foot);
    if (band >= 0) {
        int16_t start[3] = {foot[0], (int16_t)(foot[1] - 16), foot[2]};
        int16_t end[3] = {foot[0], (int16_t)(foot[1] + 16), foot[2]};
        uint16_t material;
        if (bfm_village_ground_support_probe(ground, 1, start, end, &material) & 0x2000) {
            unsigned index = actor->profile * BANDS + (unsigned)band;
            const uint8_t *row = currents->vectors + index * VECTOR_BYTES;
            for (unsigned axis = 0; axis < 3; ++axis) currents->queued[axis] = s16le(row + 2*axis);
            currents->marked = 1;
        }
    }
    if (!actor->profile && sound) *sound = current_sound(currents, occupied, foot, band >= 0);
}

void bfm_mine_currents_step(BfmMineCurrents *currents, const BfmVillageGround *ground,
    const int16_t foot[3], BfmMineCurrentSound *sound)
{
    if (sound) *sound = (BfmMineCurrentSound){0};
    for (unsigned i = 0; i < BFM_MINE_CURRENT_COUNT; ++i)
        bfm_mine_currents_visit(currents, i, (1u << BFM_MINE_CURRENT_COUNT) - 1, ground, foot, sound);
}

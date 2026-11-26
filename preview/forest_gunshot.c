/* SPDX-License-Identifier: MIT */
#include "forest_gunshot.h"
#include "forest_manual.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include "musashi_forest_player_mesh.h"

enum { GUNSHOT = 8, EMPTY_SLOT = 34, SCRIPT_BYTES = 84 };

static int read_clip(BfmGunshotClip *clip, const uint8_t *script, uint32_t skeleton,
    const BfmForestAward *award, const ForestPlayerArchive *resident,
    BfmForestGunshotPhase phase)
{
    if (!script || u32le(script) != 4 || u32le(script + 4) != skeleton ||
        u32le(script + 8) != 19) return 0;
    int marker = 0;
    for (unsigned i = 3; i < SCRIPT_BYTES / 4; ++i) {
        uint32_t value = u32le(script + 4 * i);
        if (value <= 1) {
            int loop = phase == BFM_GUNSHOT_RECOIL;
            return clip->count && !marker && value == (unsigned)loop &&
                   (clip->shot_key != 0) == loop;
        }
        if (value == 7 && phase == BFM_GUNSHOT_RECOIL && !clip->shot_key && !marker) {
            marker = 1;
            continue;
        }
        if (value < 32 || (value & 3u) || value > UINT32_MAX - 255 ||
            clip->count >= sizeof clip->pose / sizeof clip->pose[0]) return 0;
        uint8_t last;
        if (phase == BFM_GUNSHOT_EMPTY) {
            if (!forest_player_archive_read(resident, value + 255, &last, 1)) return 0;
        } else if (!bfm_forest_gunshot_bytes(award, value, 256)) return 0;
        clip->pose[clip->count++] = value + 16;
        if (marker) {
            clip->shot_key = clip->count; /* One-based; zero means no release marker. */
            marker = 0;
        }
    }
    return 0;
}

int bfm_forest_gunshot_load(BfmForestGunshot *gun, const BfmForestField *field,
    const BfmForestAward *award, uint32_t animation_table)
{
    if (!gun || !award || !award->disc ||
        animation_table > UINT32_MAX - 4 * EMPTY_SLOT) return 0;
    const uint8_t *cost = bfm_forest_room_bytes(field, UINT32_C(0x80189804), 1);
    ForestPlayerArchive resident;
    uint8_t pointer[4], empty[SCRIPT_BYTES];
    if (!cost || !(*cost & 127u) || (*cost & 128u) ||
        !forest_player_archive_open(award->disc, &resident) ||
        !forest_player_archive_read(&resident, animation_table + 4 * EMPTY_SLOT, pointer, 4) ||
        !forest_player_archive_read(&resident, u32le(pointer), empty, sizeof empty)) return 0;
    uint32_t skeleton = u32le(empty + 4);
    uint8_t parts[(MUSASHI_FOREST_PLAYER_PARTS + 1) * 4];
    if (!forest_player_archive_read(&resident, skeleton, parts, sizeof parts) ||
        u32le(parts + 4 * MUSASHI_FOREST_PLAYER_PARTS)) return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i)
        if (!u32le(parts + 4 * i)) return 0;

    BfmForestGunshot next = {.cost = *cost};
    if (!read_clip(&next.empty, empty, skeleton, award, &resident, BFM_GUNSHOT_EMPTY) ||
        !read_clip(&next.raise, bfm_forest_gunshot_bytes(award, UINT32_C(0x800cbac0), SCRIPT_BYTES),
                   skeleton, award, &resident, BFM_GUNSHOT_RAISE) ||
        !read_clip(&next.lower, bfm_forest_gunshot_bytes(award, UINT32_C(0x800cbae0), SCRIPT_BYTES),
                   skeleton, award, &resident, BFM_GUNSHOT_LOWER) ||
        !read_clip(&next.recoil, bfm_forest_gunshot_bytes(award, UINT32_C(0x800cbb00), SCRIPT_BYTES),
                   skeleton, award, &resident, BFM_GUNSHOT_RECOIL)) return 0;
    *gun = next;
    return 1;
}

static const BfmGunshotClip *active_clip(const BfmForestGunshot *gun)
{
    switch (gun->phase) {
    case BFM_GUNSHOT_RAISE: return &gun->raise;
    case BFM_GUNSHOT_RECOIL: return &gun->recoil;
    case BFM_GUNSHOT_LOWER: return &gun->lower;
    case BFM_GUNSHOT_EMPTY: return &gun->empty;
    default: return NULL;
    }
}

BfmForestGunshotEvent bfm_forest_gunshot_tick(BfmForestGunshot *gun,
    BfmForestStatus *status, int circle_pressed, int circle_held, int interrupted)
{
    if (!gun || !status || !gun->cost) return BFM_GUNSHOT_GROUND;
    uint8_t *record = status->record;
    if (interrupted || (record[0x49] & 127u) != GUNSHOT) {
        gun->phase = BFM_GUNSHOT_IDLE;
        return BFM_GUNSHOT_GROUND;
    }
    unsigned bp = u16le(record + 0x40);
    int available = bp >= gun->cost;
    const BfmGunshotClip *clip = active_clip(gun);
    if (!clip) {
        if (!circle_pressed) return BFM_GUNSHOT_GROUND;
        record[0x49] = GUNSHOT;
        record[0x4d] = available ? 0 : 128;
        gun->phase = available ? BFM_GUNSHOT_RAISE : BFM_GUNSHOT_EMPTY;
        gun->key = 0;
    } else if (++gun->key >= clip->count) {
        if (gun->phase == BFM_GUNSHOT_LOWER || gun->phase == BFM_GUNSHOT_EMPTY) {
            gun->phase = BFM_GUNSHOT_IDLE;
            return BFM_GUNSHOT_GROUND;
        }
        gun->phase = circle_held && available ? BFM_GUNSHOT_RECOIL : BFM_GUNSHOT_LOWER;
        gun->key = 0;
    }
    clip = active_clip(gun);
    gun->pose = clip->pose[gun->key];
    if (available && clip->shot_key == gun->key + 1) {
        bp -= gun->cost;
        record[0x40] = (uint8_t)bp;
        record[0x41] = (uint8_t)(bp >> 8);
        return BFM_GUNSHOT_FIRED;
    }
    return BFM_GUNSHOT_POSE;
}

int16_t bfm_forest_gunshot_aim(const BfmForestGunshot *gun, const BfmForestCamera *camera,
    int screen_x, int screen_y, int analog, int16_t yaw)
{
    int16_t target;
    if (!gun || gun->phase != BFM_GUNSHOT_RECOIL || !camera ||
        !bfm_forest_manual_heading(camera, screen_x, screen_y, &target)) return yaw;
    if (analog) return target;
    if (!target) return yaw;
    int difference = (int)target - yaw;
    if (difference > 2048) difference -= 4096;
    if (difference < -2048) difference += 4096;
    int step = difference / 4;
    if (!step) step = difference / 2;
    if (!step) step = difference;
    return (int16_t)((yaw + step) & 4095);
}

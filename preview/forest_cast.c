/* SPDX-License-Identifier: MIT */
#include "forest_cast.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include "musashi_forest_player_mesh.h"

enum { STANCE_SLOT = 29, STRAFE_SLOT = 30, THROW_SLOT = 31, AWARD_SLOT = 32 };

static int load_clip(const ForestPlayerArchive *archive, uint32_t table,
                     unsigned slot, BfmForestCastClip *clip)
{
    uint8_t pointer[4], script[104], part_table[4 * (MUSASHI_FOREST_PLAYER_PARTS + 1)];
    if (table > UINT32_MAX - 4 * slot ||
        !forest_player_archive_read(archive, table + 4 * slot, pointer, sizeof pointer) ||
        !forest_player_archive_read(archive, u32le(pointer), script, sizeof script) ||
        u32le(script) != 4 || !forest_player_archive_read(archive, u32le(script + 4),
            part_table, sizeof part_table) || u32le(part_table + 4 * MUSASHI_FOREST_PLAYER_PARTS))
        return 0;
    uint32_t visible = 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if ((u32le(part_table + 4*i) >> 24) != 0x80) return 0;
        visible |= UINT32_C(1) << i;
    }
    const uint32_t parts = visible & ~((UINT32_C(1) << 24) | (UINT32_C(1) << 25));
    uint32_t fusion = UINT32_C(1) << 22;
    int marker = 0;
    unsigned markers = 0;
    for (unsigned i = 2; i < sizeof script / 4; ++i) {
        uint32_t word = u32le(script + i * 4);
        if (word < 2)
            return clip->count && !marker && word == (slot <= STRAFE_SLOT) &&
                   markers == (slot >= THROW_SLOT) &&
                   (slot != AWARD_SLOT || (clip->hold_after && clip->hold_after < clip->count));
        switch (word) {
        case 2:
            if (slot != AWARD_SLOT || !clip->count || clip->hold_after) return 0;
            clip->hold_after = clip->count;
            break;
        case 7: marker = 1; ++markers; break;
        case 8: break; /* Strafe footstep. */
        case 16: fusion = UINT32_C(1) << 22; break;
        case 17: fusion = UINT32_C(1) << 21; break;
        case 19: break; /* Restore the ordinary off-hand equipment. */
        default: {
            uint8_t last;
            /* The model key contains a 16-byte anchor header and 240 bytes
             * of compressed joint transforms. Validate the complete extent. */
            if (word < 32 || (word & 3) || word > UINT32_MAX - 255 ||
                !forest_player_archive_read(archive, word + 255, &last, 1) ||
                clip->count >= BFM_CAST_MAX_KEYS) return 0;
            uint32_t blades = (UINT32_C(1) << 21) | (UINT32_C(1) << 22);
            clip->keys[clip->count++] = (BfmForestCastKey){
                .pose = word + 16, .visible = (parts & ~blades) | fusion, .launch = marker
            };
            marker = 0;
            break;
        }
        }
    }
    return 0;
}

int bfm_forest_cast_load(BfmForestCast *cast, const MusashiDiscMedia *disc,
                         uint32_t animation_table)
{
    ForestPlayerArchive archive;
    BfmForestCast value = {0};
    if (!cast || !disc || !forest_player_archive_open(disc, &archive) ||
        !load_clip(&archive, animation_table, STANCE_SLOT, &value.stance) ||
        !load_clip(&archive, animation_table, STRAFE_SLOT, &value.strafe) ||
        !load_clip(&archive, animation_table, THROW_SLOT, &value.throw_clip) ||
        !load_clip(&archive, animation_table, AWARD_SLOT, &value.award))
        return 0;
    value.visible = value.stance.keys[0].visible;
    *cast = value;
    return 1;
}

BfmForestCastEvent bfm_forest_cast_tick(BfmForestCast *cast, BfmForestStatus *status,
    int r1_held, int square_pressed, int moving, int blade_away)
{
    if (!cast || !status || !cast->stance.count) return BFM_CAST_GROUND;
    uint8_t *charge = status->record + 0x47;
    if (cast->active_stance_gain) {
        unsigned held_charge = cast->action == BFM_CAST_STANCE && r1_held && !blade_away
                             ? *charge + cast->active_stance_gain : 0;
        *charge = held_charge < 128 ? (uint8_t)held_charge : 128;
    }
    if (cast->action == BFM_CAST_THROW) {
        if (++cast->key >= cast->throw_clip.count)
            return cast->action = BFM_CAST_GROUND;
    } else if (!r1_held || blade_away) {
        *charge = 0;
        return cast->action = BFM_CAST_GROUND;
    } else if (square_pressed && cast->action == BFM_CAST_STANCE) {
        int full = *charge >= 128;
        *charge = 0;
        if (!full) {
            cast->action = BFM_CAST_GROUND;
            return BFM_CAST_SWING;
        }
        cast->action = BFM_CAST_THROW;
        cast->key = 0;
    } else {
        const BfmForestCastClip *clip = moving ? &cast->strafe : &cast->stance;
        if (cast->action == BFM_CAST_GROUND || cast->moving != (unsigned)!!moving)
            cast->key = 0;
        else
            cast->key = (cast->key + 1) % clip->count;
        cast->moving = !!moving;
        cast->action = BFM_CAST_STANCE;
        /* The first forest visit precedes the faster-charge story upgrade. */
        if (!cast->active_stance_gain)
            *charge = *charge > 125 ? 128 : (uint8_t)(*charge + 3);
    }
    const BfmForestCastClip *clip = cast->action == BFM_CAST_THROW ? &cast->throw_clip :
                                    cast->moving ? &cast->strafe : &cast->stance;
    const BfmForestCastKey *key = &clip->keys[cast->key];
    cast->pose = key->pose;
    /* The shoulder stance preserves the previous grip; the throw script
     * explicitly changes grips. The detached blade has its own model. */
    if (cast->action == BFM_CAST_THROW) {
        cast->visible = key->visible;
        if (blade_away)
            cast->visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 22));
    }
    return key->launch ? BFM_CAST_LAUNCH : cast->action;
}

/* SPDX-License-Identifier: MIT */
#include "steam_knight_spin_contact.h"
#include "bedroom_score.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>

int bfm_weapon_ranks_load(const MusashiDiscMedia *disc, BfmWeaponRank out[30])
{
    uint8_t ranks[30 * 16];
    if (!disc || !out ||
        !bfm_bedroom_resident_data(disc, UINT32_C(0x800638f8), ranks, sizeof ranks)) return 0;
    for (unsigned i = 0; i < 30; ++i) {
        out[i].body = u16le(ranks + 16 * i + 2);
        out[i].fusion = u16le(ranks + 16 * i + 14);
        out[i].lumina = u16le(ranks + 16 * i + 10);
        for (unsigned skill = 0; skill < 4; ++skill)
            out[i].exp_limit[skill] = u16le(ranks + 16 * i + 4 * skill);
    }
    return 1;
}

int bfm_steam_spin_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                        size_t bytes, BfmSteamSpinData *out)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t move_offset = UINT32_C(0x80192908) - base;
    if (!disc || !scene || !out || bytes < move_offset + 20) return 0;
    const uint8_t *move = scene + move_offset;
    uint32_t kind = u32le(move);
    if (kind != 12 || u32le(move + 4) != 2 || u16le(move + 18) != 2) return 0;

    size_t power_offset = (UINT32_C(0x80192944) - base) + 12 * kind;
    if (bytes < power_offset + 12) return 0;
    const uint8_t *power = scene + power_offset;
    uint16_t mode = u16le(power);
    if ((mode & 0xf000) != 0xc000 || u16le(power + 8) || u16le(power + 10) != 2) return 0;

    BfmSteamSpinData data = {
        .fusion_percent = mode & 0xfff, .fusion_spread = u16le(power + 2),
        .lumina_percent = u16le(power + 4), .lumina_spread = u16le(power + 6),
        .target_mask = u16le(move + 18)
    };
    if (!bfm_weapon_ranks_load(disc, data.ranks)) return 0;
    *out = data;
    return 1;
}

static unsigned varied_percent(BfmGameplayRun *run, unsigned percent, unsigned spread)
{
    return spread ? percent + bfm_gameplay_random(run) % spread : percent;
}

static int critical_roll(BfmGameplayRun *run, uint32_t actor_flags, int progress_critical)
{
    if ((actor_flags & 0x200) && (bfm_gameplay_random(run) & 1)) return 1;
    if (progress_critical && !(bfm_gameplay_random(run) & 3)) return 1;
    if (bfm_gameplay_random(run) & 3) return 0;
    unsigned draw = bfm_gameplay_random(run) % 160;
    return draw >= 81 && (int)draw < s16le(run->player.record + 0x2e);
}

static unsigned rank_cap(int64_t progress)
{
    if (progress < 10) return 0;
    if (progress < 200) return 8;
    if (progress < 600) return 16;
    if (progress < 900) return 22;
    if (progress < 1200) return 27;
    return 99;
}

int bfm_weapon_roll(const BfmWeaponRank ranks[30], const BfmWeaponPower *power,
                    BfmGameplayRun *run, uint32_t actor_flags, BfmSteamSpinRoll *out)
{
    if (!ranks || !power || !run || !run->has_player || !out) return 0;
    const uint8_t *status = run->player.record;
    uint32_t body = u32le(status + 0x6c), fusion = u32le(status + 0x7c);
    uint32_t lumina = u32le(status + 0x84);
    uint32_t mind = u32le(status + 0x74);
    if (body >= 30 || mind >= 30 || fusion >= 30 || lumina >= 30) return 0;
    uint32_t story = u32le(status + 8);
    int64_t progress = (int64_t)(story ^ UINT32_C(0x80000000)) - INT64_C(0x80000000);
    unsigned cap = rank_cap(progress);
    const unsigned levels[] = {body, mind, lumina, fusion};
    const unsigned exp_offsets[] = {0x70, 0x78, 0x88, 0x80};
    int progress_critical = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (levels[i] + 1 >= cap &&
            u32le(status + exp_offsets[i]) == ranks[levels[i]].exp_limit[i]) {
            progress_critical = 1;
            break;
        }
    unsigned body_stat = ranks[body].body;
    unsigned fusion_base = (body_stat + ranks[fusion].fusion) *
                           (bfm_gameplay_story_flag(run, 0x81) ? 150u : 125u) / 100;
    unsigned lumina_base = (body_stat + ranks[lumina].lumina) * 175u / 100;
    unsigned first = varied_percent(run, power->fusion_percent, power->fusion_spread);
    unsigned second = varied_percent(run, power->lumina_percent, power->lumina_spread);
    uint64_t raw = (uint64_t)fusion_base * first / 100 + (uint64_t)lumina_base * second / 100;
    int critical = power->can_critical && critical_roll(run, actor_flags, progress_critical);
    if (critical) raw = raw * (bfm_gameplay_story_flag(run, 0x1c) ? 175u : 150u) / 100;
    *out = (BfmSteamSpinRoll){.strength = (uint16_t)(raw & 0x7fff), .critical = critical};
    return 1;
}

int bfm_steam_spin_roll(const BfmSteamSpinData *data, BfmGameplayRun *run,
                        uint32_t actor_flags, BfmSteamSpinRoll *out)
{
    if (!data || !run || !run->has_player || !out || (run->player.record[0x48] & 0x7f) != 1)
        return 0;
    BfmWeaponPower power = {data->fusion_percent, data->fusion_spread,
                            data->lumina_percent, data->lumina_spread, 1};
    return bfm_weapon_roll(data->ranks, &power, run, actor_flags, out);
}

static int rotate_axis(const int16_t row[3], const int point[3])
{
    int64_t dot = 0;
    for (unsigned axis = 0; axis < 3; ++axis)
        dot += (int64_t)row[axis] * point[axis];
    return (int)(dot / 4096 - (dot % 4096 < 0));
}

static void world_point(const BfmForestTransform *actor, const int local[3], int16_t world[3])
{
    for (unsigned i = 0; i < 3; ++i) {
        uint16_t word = (uint16_t)((int64_t)actor->translation[i] +
                                   rotate_axis(actor->rotation[i], local));
        world[i] = (int16_t)((int32_t)(word ^ 0x8000u) - 0x8000);
    }
}

int bfm_steam_spin_segments(const BfmForestTransform *actor,
                            const BfmForestTransform *previous_in_current,
                            BfmSteamSpinSegments *out)
{
    if (!actor || !previous_in_current || !out) return 0;
    const int pivot[3] = {0, -30, 0};
    const int tip[3] = {0, -30, -82};
    int previous[3], third[3];
    int64_t length_squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        previous[i] = rotate_axis(previous_in_current->rotation[i], tip);
        int delta = tip[i] - previous[i];
        third[i] = delta / 3;
        length_squared += (int64_t)delta * delta;
    }
    BfmSteamSpinSegments segments = {.count = 1};
    world_point(actor, pivot, segments.pivot);
    world_point(actor, previous, segments.ends[0]);
    /* In bank two, the original integer distance first reaches 65 at this
     * squared chord length. A 65-unit axis-aligned chord still reports 64. */
    if (length_squared >= 4288) {
        int near[3], far[3];
        for (unsigned i = 0; i < 3; ++i) {
            near[i] = previous[i] + third[i];
            far[i] = tip[i] - third[i];
        }
        world_point(actor, near, segments.ends[1]);
        world_point(actor, far, segments.ends[2]);
        segments.count = 3;
    }
    *out = segments;
    return 1;
}

int bfm_steam_spin_segments_yaw(const BfmForestTransform *actor, int16_t previous_yaw,
                                int16_t current_yaw, BfmSteamSpinSegments *out)
{
    unsigned phase = (unsigned)(previous_yaw - current_yaw) & 4095u;
    double angle = phase * (6.28318530717958647693 / 4096);
    int16_t sine = (int16_t)lround(4096 * sin(angle));
    int16_t cosine = (int16_t)lround(4096 * cos(angle));
    BfmForestTransform relative = {.rotation = {
        {cosine, 0, sine}, {0, 4096, 0}, {-sine, 0, cosine}
    }};
    return bfm_steam_spin_segments(actor, &relative, out);
}

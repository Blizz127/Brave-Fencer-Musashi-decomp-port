/* SPDX-License-Identifier: MIT */
#include "forest_bank_thorn_hurt.h"
#include <math.h>
#include <string.h>

enum { Q12 = 4096, Q16 = 65536, RECOIL = 2, RETURN = 3, SEEK_FLOOR = 4 };
static const double radians = 6.28318530717958647693 / Q12;

static int floor_div(int64_t value, int scale)
{
    return (int)(value / scale - (value % scale < 0));
}

static void sin_cos(int angle, int *sine, int *cosine)
{
    double turn = ((unsigned)angle & 4095u) * radians;
    *sine = (int)lround(Q12 * sin(turn));
    *cosine = (int)lround(Q12 * cos(turn));
}

/* The field restores the length of each basis column after each model turn.
 * Only the forward column contributes to this reaction. */
static void normalize(int v[3])
{
    unsigned magnitude = (unsigned)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    unsigned divisor = 8;
    while (magnitude > 255) {
        magnitude >>= 2;
        divisor *= 2;
    }
    int gain = (int)sqrt((double)(1u << 30) / magnitude);
    for (int i = 0; i < 3; ++i)
        v[i] = floor_div(v[i] * gain, (int)divisor);
}

static void recoil(BfmForestBankThornHurt *hurt, const BfmForestBankThornInput *in)
{
    int sp, cp, sy, cy;
    sin_cos(in->pitch, &sp, &cp);
    sin_cos(in->yaw, &sy, &cy);
    int forward[3] = {0, -sp, cp};
    normalize(forward);
    forward[0] = sy;
    forward[1] = floor_div(forward[1] * cy, Q12);
    forward[2] = floor_div(forward[2] * cy, Q12);
    normalize(forward);
    /* Local roll rotates the two other columns; the field still normalizes
     * forward again as part of that turn. */
    normalize(forward);
    for (int axis = 0; axis < 3; ++axis) {
        int speed = forward[axis] * 64;
        if (in->motion_flags & (UINT32_C(0x400000) << axis))
            speed = axis == 2 ? 4 * Q16 : 0;
        in->position[axis] += speed;
    }
    hurt->vertical_speed += Q16 + Q16/2;
    in->position[1] += hurt->vertical_speed;
}

static void offset_y(int16_t out[3], const int16_t from[3], int distance)
{
    out[0] = from[0];
    out[1] = (int16_t)(from[1] + distance);
    out[2] = from[2];
}

void bfm_forest_bank_thorn_remember(BfmForestBankThornHurt *hurt,
    const BfmVillageGround *ground, const int16_t foot[3])
{
    if (!hurt || hurt->phase || !ground || !foot) return;
    int16_t below[3];
    uint16_t material;
    offset_y(below, foot, 8);
    if (bfm_village_ground_support_probe(ground, 1, foot, below, &material) == 0x2000 &&
        !(material & 255u) && !memcmp(foot, below, sizeof below))
        memcpy(hurt->safe, foot, sizeof hurt->safe);
}

static int settle(int64_t position[3], const int16_t safe[3])
{
    int arrived = 1;
    for (int i = 0; i < 3; ++i) {
        int world = floor_div(position[i], Q16);
        int distance = safe[i] - world;
        int step = distance / 8;
        if (!step) step = (distance > 0) - (distance < 0);
        position[i] += (int64_t)step * Q16;
        if (world + step != safe[i]) arrived = 0;
    }
    return arrived;
}

static int landing(const BfmVillageGround *ground, const int16_t safe[3],
                   unsigned ring, int16_t foot[3])
{
    const int radius = (int)ring * 4;
    int16_t bank[3];
    offset_y(bank, safe, -4);
    for (int angle = 0; angle < Q12; angle += 32) {
        int sine, cosine;
        sin_cos(angle, &sine, &cosine);
        int16_t probe[3] = {
            (int16_t)(safe[0] + floor_div(radius * sine, Q12)),
            (int16_t)(safe[1] - radius),
            (int16_t)(safe[2] + floor_div(radius * cosine, Q12))
        };
        int16_t surface[3], arrival[3], check[3];
        uint16_t material;
        offset_y(surface, probe, 31);
        if (bfm_village_ground_support_probe(ground, 0, probe, surface, &material) != 0x2000 ||
            (material & 255u)) continue;
        memcpy(arrival, surface, sizeof arrival);
        if (bfm_village_ground_support_probe(ground, 0, bank, arrival, &material) != 0x2000)
            continue;
        offset_y(surface, arrival, 8);
        if (bfm_village_ground_support_probe(ground, 0, arrival, surface, &material) != 0x2000)
            continue;
        offset_y(check, surface, 6);
        if (bfm_village_ground_support_probe(ground, 2, surface, check, &material) == 0x2000) {
            memcpy(foot, surface, sizeof surface);
            return 1;
        }
    }
    return 0;
}

static int start(BfmForestBankThornHurt *hurt, BfmGameplayRun *run,
                 const BfmForestBankThornInput *in, BfmForestBankThornFrame *frame)
{
    if (!in->incoming || in->incoming->flags != 0xc007 || in->incoming->power != 32 ||
        (in->scene_state & UINT32_C(0x80ffffff)) || in->player_action == 25 ||
        in->player_action == 26 || (in->protected_action && !(in->protected_action & 0x02000000)))
        return 1;
    BfmForestStatusValues status = bfm_forest_status_values(&run->player);
    if (status.hp <= 0) return 1;
    if (!bfm_forest_hit_damage(in->disc, &run->player, in->player_flags, in->incoming->power,
                               &hurt->damage)) return 0;
    if (run->player.record[0x49] == 20) {
        int bp = status.bp + 10;
        if (bp > status.max_bp) bp = status.max_bp;
        run->player.record[0x40] = (uint8_t)bp;
        run->player.record[0x41] = (uint8_t)((unsigned)bp >> 8);
        frame->bp_credit = 10;
    }
    frame->started = 1;
    frame->movement_locked = 1;
    for (int axis = 0; axis < 3; ++axis)
        frame->effect_root[axis] = (int16_t)floor_div(in->position[axis], Q16);
    frame->damage_number = hurt->damage;
    frame->rumble_program = UINT32_C(0x80062c04);
    frame->sound = 1105;
    frame->body_program = UINT32_C(0x800d5880);
    hurt->vertical_speed = -16 * Q16;
    hurt->ticks = 17;
    hurt->phase = RECOIL;
    return 1;
}

int bfm_forest_bank_thorn_hurt_step(BfmForestBankThornHurt *hurt, BfmGameplayRun *run,
    const BfmForestBankThornInput *in, BfmForestBankThornFrame *frame)
{
    if (!hurt || !run || !in || !in->position || !in->ground || !in->entrance || !frame)
        return 0;
    *frame = (BfmForestBankThornFrame){.yaw = (unsigned)in->yaw & 4095u};
    if (!hurt->phase) {
        if (!start(hurt, run, in, frame)) return 0;
    }
    if (hurt->phase) {
        frame->movement_locked = 1;
        frame->hide_body = 1;
        if (hurt->phase == RECOIL) {
            recoil(hurt, in);
            if (--hurt->ticks) frame->hide_body = (int)(in->field & 1u);
            else hurt->phase = RETURN;
        } else if (hurt->phase == RETURN) {
            if (settle(in->position, hurt->safe)) {
                hurt->phase = SEEK_FLOOR;
                hurt->ticks = 1;
            }
        } else if (hurt->phase == SEEK_FLOOR) {
            int16_t foot[3];
            if (hurt->ticks <= 32u && !landing(in->ground, hurt->safe, hurt->ticks, foot)) {
                ++hurt->ticks;
            } else {
                if (hurt->ticks > 32u) {
                    memcpy(foot, in->entrance->xyz, sizeof foot);
                    frame->used_entrance = 1;
                    frame->yaw = in->entrance->yaw;
                }
                for (int i = 0; i < 3; ++i) in->position[i] = (int64_t)foot[i] * Q16;
                unsigned damage = (in->player_flags & UINT32_C(0x02000000))
                    ? bfm_forest_status_values(&run->player).max_hp : hurt->damage;
                bfm_forest_status_debit(&run->player, damage);
                frame->zero_hp = bfm_forest_status_values(&run->player).hp <= 0;
                frame->recovered = 1;
                frame->hide_body = 0;
                frame->blink_fields = 32;
                unsigned ability = run->player.record[0x49];
                if (ability == 3 || ability == 12 || ability == 24) {
                    frame->ended_ability = ability;
                    memset(run->player.record + 0x49, 0, 4);
                }
                hurt->phase = 0;
            }
        } else return 0;
    }
    frame->action = hurt->phase ? 25 : frame->recovered ? 0 : in->player_action;
    frame->phase = hurt->phase;
    return 1;
}

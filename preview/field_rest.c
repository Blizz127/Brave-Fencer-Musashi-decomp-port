/* SPDX-License-Identifier: MIT */
#include "field_rest.h"
#include "../pc_port/forest_player_bytes.h"

static void begin_pose(BfmFieldRest *rest, BfmFieldRestPose pose)
{
    rest->pose = pose;
    rest->ticks = 0;
}

void bfm_field_rest_input(BfmFieldRest *rest, BfmSaveRecord *record, BfmFieldInput input)
{
    uint8_t *charge = &record->progress[0x47];
    uint8_t *weapon = &record->progress[0x48];
    switch (rest->pose) {
    case BFM_FIELD_REST_FREE:
        if (input.select_pressed) *weapon ^= 0x80;
        if ((*weapon & 0x80) && input.shoulder_held) {
            *charge = 0;
            begin_pose(rest, BFM_FIELD_REST_CHARGE);
        }
        break;
    case BFM_FIELD_REST_CHARGE:
        if (!input.shoulder_held) {
            begin_pose(rest, *charge == 128 ? BFM_FIELD_REST_DROP : BFM_FIELD_REST_WAKE);
        } else {
            unsigned next = *charge + ((record->flags[32 / 8] & (1u << (32 % 8))) ? 6u : 3u);
            *charge = next > 128 ? 128 : (uint8_t)next;
        }
        break;
    case BFM_FIELD_REST_ASLEEP:
        if (input.cross_pressed || input.circle_pressed || input.triangle_pressed || input.square_pressed)
            begin_pose(rest, BFM_FIELD_REST_WAKE);
        break;
    case BFM_FIELD_REST_DROP:
    case BFM_FIELD_REST_WAKE:
        *charge = 0;
        break;
    }
}

void bfm_field_rest_advance(BfmFieldRest *rest, int pose_finished)
{
    if (pose_finished && rest->pose == BFM_FIELD_REST_DROP)
        begin_pose(rest, BFM_FIELD_REST_ASLEEP);
    else if (pose_finished && rest->pose == BFM_FIELD_REST_WAKE)
        begin_pose(rest, BFM_FIELD_REST_FREE);
    else
        ++rest->ticks;
}

int bfm_field_rest_sleeping(const BfmFieldRest *rest)
{
    return rest->pose == BFM_FIELD_REST_DROP || rest->pose == BFM_FIELD_REST_ASLEEP;
}

static void write_reserve(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void recover(uint8_t *p, unsigned limit, unsigned step)
{
    /* A field nap leaves a better reserve alone. The last sub-unit gain
     * can retain a fraction above the maximum whole value. */
    if (s16le(p + 2) < (int)limit)
        write_reserve(p, u32le(p) + step);
}

void bfm_field_rest_stamina(BfmSaveRecord *record, int asleep)
{
    uint8_t *p = record->progress;
    if (asleep) {
        int upgraded = record->flags[27 / 8] & (1u << (27 % 8));
        unsigned limit = upgraded ? 160u : 128u;
        unsigned step = upgraded ? 11648u : 5824u;
        recover(p + 0x2c, limit, step);
        recover(p + 0x60, limit, step);
    } else {
        unsigned step = (record->flags[33 / 8] & (1u << (33 % 8))) ? 242u : 364u;
        uint32_t reserve = u32le(p + 0x2c);
        write_reserve(p + 0x2c, s16le(p + 0x2e) >= 0 && reserve > step ? reserve - step : 0);
    }
}

/* SPDX-License-Identifier: MIT */
#include "forest_damage_digits.h"
#include "forest_opening_data.h"
#include <limits.h>

enum { rise_updates = 11, hold_updates = 8, fade_updates = 7 };

int bfm_forest_damage_digits_start(BfmForestDamageDigits *digits, const BfmForestField *field,
                                   const int16_t contact[3], uint16_t amount)
{
    if (!digits || !contact) return 0;
    const uint8_t *palette = bfm_forest_room_bytes(field, UINT32_C(0x80189c80), 1);
    return bfm_forest_damage_digits_start_data(digits, palette, contact, amount);
}

int bfm_forest_damage_digits_start_data(BfmForestDamageDigits *digits, const uint8_t *palette,
                                        const int16_t contact[3], uint16_t amount)
{
    if (!digits || !palette || !contact) return 0;
    *digits = (BfmForestDamageDigits){
        .contact = {contact[0], contact[1], contact[2]}, .amount = amount, .palette = *palette
    };
    return 1;
}

int bfm_forest_damage_digits_prepare_hurt(BfmForestDamageDigits *digits, const BfmForestField *field,
                                          const int16_t hero[3], uint16_t amount)
{
    if (!digits || !hero || amount > 999) return 0;
    const uint8_t *palette = bfm_forest_room_bytes(field, UINT32_C(0x80189c80), 2);
    if (!palette) return 0;
    *digits = (BfmForestDamageDigits){
        .contact = {hero[0], hero[1], hero[2]}, .amount = amount, .palette = palette[1], .hurt = 1
    };
    return 1;
}

void bfm_forest_damage_digits_scatter(BfmForestDamageDigits *digits, uint16_t random_result)
{
    digits->drift_x = (int8_t)((int)(random_result & 15) - 8);
    digits->drift_z = (int8_t)((int)((random_result >> 4) & 15) - 8);
}

int bfm_forest_damage_digits_start_hurt(BfmForestDamageDigits *digits, const BfmForestField *field,
                                        const int16_t hero[3], uint16_t amount, uint16_t random_result)
{
    if (!bfm_forest_damage_digits_prepare_hurt(digits, field, hero, amount)) return 0;
    bfm_forest_damage_digits_scatter(digits, random_result);
    return 1;
}

void bfm_forest_damage_digits_advance(BfmForestDamageDigits *digits)
{
    if (!digits || !digits->amount) return;
    if (++digits->age > rise_updates + hold_updates + fade_updates) digits->amount = 0;
}

int bfm_forest_damage_digits_last(const BfmForestDamageDigits *digits)
{
    return digits->age >= rise_updates + hold_updates + fade_updates;
}

static int floor_divide(int64_t n, int d)
{
    return (int)(n / d - (n % d < 0));
}

/* Project the rising world anchor. The glyph width stays fixed in pixels. */
static int screen_anchor(const BfmForestCamera *camera, const int world[3],
                         int *screen_x, int *screen_y)
{
    int64_t view[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        const int16_t *r = camera->rotation[axis];
        int64_t dot = (int64_t)r[0] * world[0] + (int64_t)r[1] * world[1] +
                      (int64_t)r[2] * world[2];
        view[axis] = (int64_t)camera->translation[axis] + floor_divide(dot, 4096);
    }
    if (view[0] < INT16_MIN || view[0] > INT16_MAX || view[1] < INT16_MIN ||
        view[1] > INT16_MAX || view[2] <= 0 || view[2] > INT16_MAX ||
        camera->projection >= 2 * view[2]) return 0;
    int64_t scale = ((int64_t)camera->projection * 65536 + view[2] / 2) / view[2];
    *screen_x = 160 + floor_divide(view[0] * scale, 65536);
    *screen_y = 120 + floor_divide(view[1] * scale, 65536);
    return *screen_x >= -1024 && *screen_x <= 1023 && *screen_y >= -1024 && *screen_y <= 1023;
}

unsigned bfm_forest_damage_digits_project(const BfmForestDamageDigits *digits,
                                         const BfmForestCamera *camera,
                                         BfmForestDustCard out[BFM_FOREST_DAMAGE_DIGIT_CARDS])
{
    if (!digits || !digits->amount || !digits->age || !camera || !camera->projection || !out)
        return 0;
    int age = digits->age - 1;
    int rise = age < rise_updates ? age : rise_updates;
    int fade = age > rise_updates + hold_updates ? age - rise_updates - hold_updates : 0;
    /* A BP credit is issued 32 units above the hero, then its own
     * number initializer lifts the first card by a further 24. */
    int lift = (digits->bp_credit ? 56 : 32) + rise * (2 * rise_updates + 1 - rise) / 2;
    int world[3] = {digits->contact[0], digits->contact[1] - lift, digits->contact[2]};
    if (digits->hurt) {
        /* This number drifts sideways and follows a vertical arc in addition
         * to the initial rise. Gravity adds 1.5 units per update. */
        world[0] += floor_divide(digits->drift_x * age, 2);
        world[1] += -10 * age + 3 * age * (age - 1) / 4;
        world[2] += floor_divide(digits->drift_z * age, 2);
    } else {
        world[1] -= 2 * fade;
    }
    int x, y;
    if (!screen_anchor(camera, world, &x, &y)) return 0;

    unsigned shade;
    int half_height;
    int luminous = age >= rise_updates + hold_updates;
    int full_height = digits->hurt || digits->bp_credit;
    if (age < rise_updates) {
        shade = full_height ? 128 : 96;
        half_height = full_height ? 4 : 1 + (age + 2) / 4;
    } else if (!luminous) {
        shade = (unsigned)((full_height ? 192 : 255) - 8 * (age - rise_updates));
        half_height = 4;
    } else {
        shade = (unsigned)(112 - 16 * fade);
        half_height = 5 + fade;
    }
    unsigned value = digits->amount;
    unsigned places = value < 10 ? 1 : value < 100 ? 2 : value < 1000 ? 3 : 4;
    if (digits->hurt) ++places;
    if (places > BFM_FOREST_DAMAGE_DIGIT_CARDS) return 0;
    /* Submit units first. The ordering table draws the leftmost glyph first. */
    int left = x + 8 - 4 * (int)places;
    unsigned palette_x = digits->palette < 224 ? 352 : 256;
    for (unsigned i = 0; i < places; ++i) {
        BfmForestDustCard *card = &out[i];
        unsigned symbol = digits->hurt && i + 1 == places ? 10 : value % 10;
        unsigned u = 40 + 8 * symbol;
        value /= 10;
        card->slot = i;
        card->bucket = 16;
        card->material = (BfmForestRasterMaterial){
            .page = (uint16_t)(0x16 | (luminous ? 0x20 : 0)),
            .clut = (uint16_t)(((256u + digits->palette) << 6) | (palette_x / 16)),
            .count = 4, .textured = 1, .translucent = luminous
        };
        for (unsigned k = 0; k < 4; ++k) {
            int right = (int)(k & 1), bottom = (int)(k >> 1);
            card->vertices[k] = (BfmForestRasterVertex){
                left + right * 8, y + (bottom ? half_height : -half_height),
                (uint8_t)(u + right * 8), (uint8_t)(208 + bottom * 8), shade * 0x010101u
            };
        }
        left -= 8;
    }
    return places;
}

/* SPDX-License-Identifier: MIT */
#include "tower_summit_departure.h"
#include <limits.h>
#include <string.h>

int bfm_tower_summit_departure_camera(BfmForestCamera *camera, int32_t eye[3])
{
    const int32_t fixed_eye[3] = {1033, -2112, 479};
    const int32_t target[3] = {297, -1426, 54};
    int32_t near_target[3];
    /* Preserve the ray to the lift inside the field divider's short range. */
    for (unsigned i = 0; i < 3; ++i)
        near_target[i] = fixed_eye[i] + (target[i] - fixed_eye[i]) / 2;
    if (!bfm_forest_camera_look_at(fixed_eye, near_target, 400, camera)) return 0;
    if (eye) memcpy(eye, fixed_eye, sizeof fixed_eye);
    return 1;
}

static int q12(int value)
{
    return value / 4096 - (value % 4096 < 0);
}

void bfm_tower_summit_departure_hero(unsigned age, BfmForestTransform *root)
{
    if (!root || age <= 46) return;
    unsigned step = age - 46;
    if (step > 32) step = 32;
    int width = 4096 - 128 * (int)step;
    int height = 4096 + 256 * (int)step;
    for (unsigned row = 0; row < 3; ++row) {
        for (unsigned column = 0; column < 3; ++column) {
            int value = q12(root->rotation[row][column] * (column == 1 ? height : width));
            if (value < INT16_MIN) value = INT16_MIN;
            if (value > INT16_MAX) value = INT16_MAX;
            root->rotation[row][column] = (int16_t)value;
        }
    }
}

typedef struct {
    int16_t xyz[3];
    uint32_t rgb;
} LiftVertex;

static void submit(const BfmForestCamera *camera, const LiftVertex *a, const LiftVertex *b,
                   const LiftVertex *c, const LiftVertex *d, BfmForestDustCard *out, size_t *count)
{
    const LiftVertex *corner[4] = {a, b, c, d};
    int16_t xyz[4][3];
    for (unsigned i = 0; i < 4; ++i) memcpy(xyz[i], corner[i]->xyz, sizeof xyz[i]);
    BfmForestDustCard card = {0};
    if (!bfm_forest_project_effect_quad(camera, xyz, &card)) return;
    if (card.bucket > 4095) card.bucket = 4095;
    card.material = (BfmForestRasterMaterial){.count=4, .page=32, .translucent=1};
    for (unsigned i = 0; i < 4; ++i) card.vertices[i].rgb = corner[i]->rgb;
    out[(*count)++] = card;
}

size_t bfm_tower_summit_departure_cards(const uint8_t *scene, size_t bytes,
                                        const int16_t pad[3], unsigned age, unsigned tick,
                                        const BfmForestCamera *camera,
                                        BfmForestDustCard *out, size_t capacity)
{
    /* The US indoor scene supplies the closed perimeter as signed X/Z pairs. */
    enum { ring = 0x620e0, sectors = 20, full = 2688 };
    if (!scene || bytes < ring + 2*(sectors+1) || !pad || !camera || !out ||
        capacity < BFM_TOWER_SUMMIT_DEPARTURE_CARDS || age == 0 || age >= 90) return 0;
    int scale = age < 12 ? (int)age * 224 : full;
    if (age > 79) scale -= (int)(age - 79) * 256;
    unsigned wall_step = age > 13 ? age - 13 : 0;
    if (wall_step > 32) wall_step = 32;
    int y = pad[1] - 8, top_y = y + q12(-(int)wall_step * 16 * scale);
    /* Every mounted signed byte is at most 128 units from its local origin. */
    if (top_y < INT16_MIN || pad[0] < INT16_MIN+256 || pad[0] > INT16_MAX-256 ||
        pad[2] < INT16_MIN+256 || pad[2] > INT16_MAX-256) return 0;
    unsigned intensity = 32u + (unsigned)scale / 32;
    unsigned red = (tick & 1) ? intensity / 2 : intensity;
    uint32_t color = (intensity << 16) | (intensity << 8) | red;
    LiftVertex center = {{pad[0], (int16_t)y, pad[2]}, 0};
    LiftVertex inner[sectors+1], outer[sectors+1], upper[sectors+1];
    for (unsigned i = 0; i <= sectors; ++i) {
        int x = scene[ring + 2*i], z = scene[ring + 2*i+1];
        if (x >= 128) x -= 256;
        if (z >= 128) z -= 256;
        inner[i] = (LiftVertex){{pad[0]+q12(x*scale), y, pad[2]+q12(z*scale)}, color};
        outer[i] = (LiftVertex){{pad[0]+q12(2*x*scale), y, pad[2]+q12(2*z*scale)}, 0};
        upper[i] = inner[i];
        upper[i].xyz[1] = (int16_t)top_y;
        upper[i].rgb = 0;
    }
    size_t count = 0;
    for (unsigned i = 0; i < sectors; i += 2)
        submit(camera, &inner[i], &center, &inner[i+1], &inner[i+2], out, &count);
    for (unsigned i = 0; i < sectors; ++i)
        submit(camera, &outer[i], &outer[i+1], &inner[i], &inner[i+1], out, &count);
    if (wall_step) {
        for (unsigned i = 0; i < sectors; ++i)
            submit(camera, &upper[i], &upper[i+1], &inner[i], &inner[i+1], out, &count);
    }
    return count;
}

/* SPDX-License-Identifier: MIT */
#include "forest_impact.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <math.h>

static int64_t down(int64_t value, unsigned bits)
{
    int64_t unit = INT64_C(1) << bits;
    return value / unit - (value % unit < 0);
}

/* This strike uses the two single-update sprite sequences in the mounted
 * room. Bind the complete sequence so an invalid late cell cannot escape the
 * atlas page after the effect has begun. */
static int animation(BfmForestImpactAnimation *out, const BfmForestField *field,
                     uint32_t sprite, uint32_t sequence, unsigned count)
{
    const uint8_t *d = bfm_forest_room_bytes(field, sprite, 12);
    const uint8_t *cells = bfm_forest_room_bytes(field, sequence, count * 8);
    if (!d || !cells || !d[2] || !d[3]) return 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *c = cells + 8 * i;
        unsigned tx = u16le(c), ty = u16le(c + 2);
        if (u32le(c + 4) != (i + 1 == count ? 193u : 1u) ||
            tx >= 1024 || ty >= 512 || (tx % 64) * 4 + d[2] > 256 ||
            ty % 256 + d[3] > 256) return 0;
    }
    *out = (BfmForestImpactAnimation){d, cells, count};
    return 1;
}

static void toward_eye(const int32_t origin[3], const int32_t eye[3], int32_t result[3])
{
    int delta[3];
    unsigned squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t d = (int64_t)eye[i] - origin[i];
        if (d < -16383 || d > 16383) {
            for (unsigned j = 0; j < 3; ++j) result[j] = origin[j];
            result[1] -= 64;
            return;
        }
        delta[i] = (int)d;
        squared += (unsigned)(delta[i] * delta[i]);
    }
    unsigned shift = 9;
    while (squared > 255) { squared >>= 2; ++shift; }
    int gain = squared ? (int)sqrt((UINT32_C(1) << 30) / squared) : 0;
    for (unsigned i = 0; i < 3; ++i)
        result[i] = origin[i] + down((int64_t)delta[i] * gain, shift);
}

static int streak_angle(int64_t y, int64_t x)
{
    return (x || y) ? (int)lround(atan2((double)y, (double)x) * (2048.0 / acos(-1.0))) : 0;
}

static void impact_face_normal(const int16_t surface[3], int *pitch, int *yaw)
{
    int direction[3];
    for (unsigned k = 0; k < 3; ++k) {
        unsigned value = ((uint16_t)surface[k] >> 3) & 1023u;
        direction[k] = (value < 512u ? (int)value : (int)value - 1024) * 8;
    }
    int horizontal = (int)hypot(direction[0], direction[2]);
    *pitch = -streak_angle(direction[1], horizontal);
    *yaw = streak_angle(direction[0], direction[2]);
}

/* Sample the same quarter wave as the other field actors. Negative angles
 * reflect the positive sample, including the shared quarter-wave endpoints. */
static void streak_wave(int angle, int *sine, int *cosine)
{
    const int16_t zero[3] = {0};
    BfmForestTransform turn;
    bfm_forest_actor_root((unsigned)(angle < 0 ? -angle : angle), zero, &turn);
    *sine = angle < 0 ? -turn.rotation[0][2] : turn.rotation[0][2];
    *cosine = turn.rotation[0][0];
}

static void impact_strip_setup(int16_t orientation[3][3], int pitch, int yaw, int tilt,
                              const int32_t eye[3], const int32_t origin[3])
{
    int sp, cp, sy, cy;
    streak_wave(pitch + tilt, &sp, &cp);
    streak_wave(yaw - 1024, &sy, &cy);
    /* The plane is aligned to the struck face. Its short axis turns toward
     * the camera once, at creation. The basis is fixed for the entire strip. */
    int basis[3][3] = {
        {cy, down(sy * sp, 12), down(sy * cp, 12)},
        {0, cp, -sp},
        {-sy, down(cy * sp, 12), down(cy * cp, 12)}
    };
    int64_t bearing[2];
    for (unsigned axis = 0; axis < 2; ++axis) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k)
            dot += ((int64_t)eye[k] - origin[k]) * basis[k][axis];
        bearing[axis] = down(dot, 12);
    }
    int sr, cr;
    streak_wave(-streak_angle(bearing[0], bearing[1]), &sr, &cr);
    for (unsigned row = 0; row < 3; ++row) {
        int x = basis[row][0], y = basis[row][1];
        orientation[row][0] = (int16_t)basis[row][2];
        orientation[row][1] = (int16_t)(-down(x * cr, 12) -
                                         down(y * sr, 12));
        orientation[row][2] = (int16_t)(down(x * sr, 12) -
                                         down(y * cr, 12));
    }
}

static int impact_strip_project(const int16_t orientation[3][3], const int32_t origin[3],
                                const BfmForestCamera *cam, int left, int top, int width,
                                int height, BfmForestDustCard *card)
{
    int64_t root[3];
    int screen_axes[3][2];
    for (unsigned row = 0; row < 3; ++row) {
        int64_t center = 0, wide = 0, narrow = 0;
        for (unsigned k = 0; k < 3; ++k) {
            int weight = cam->rotation[row][k];
            center += (int64_t)weight * origin[k];
            wide += (int64_t)weight * orientation[k][0];
            narrow += (int64_t)weight * orientation[k][1];
        }
        root[row] = (int64_t)cam->translation[row] + down(center, 12);
        screen_axes[row][0] = down(wide * 5, 12);
        screen_axes[row][1] = down(narrow * 2, 12);
    }
    unsigned depth_sum = 0;
    for (unsigned corner = 0; corner < 4; ++corner) {
        int x = left + ((corner & 1u) ? width : 0);
        int y = top + ((corner & 2u) ? height : 0);
        int64_t view[3];
        for (unsigned k = 0; k < 3; ++k)
            view[k] = root[k] + down((int64_t)x * screen_axes[k][0] +
                                     (int64_t)y * screen_axes[k][1], 12);
        if (view[0] < INT16_MIN || view[0] > INT16_MAX || view[1] < INT16_MIN ||
            view[1] > INT16_MAX || view[2] <= 0 || view[2] > 65535 ||
            cam->projection >= 2 * view[2])
            return 0;
        int64_t scale = ((int64_t)cam->projection * 65536 + view[2] / 2) / view[2];
        int sx = 160 + down(view[0] * scale, 16);
        int sy = 120 + down(view[1] * scale, 16);
        if (sx < -1024 || sx > 1023 || sy < -1024 || sy > 1023)
            return 0;
        card->vertices[corner].x = sx;
        card->vertices[corner].y = sy;
        depth_sum += (unsigned)view[2];
    }
    card->bucket = depth_sum / 16 + 1;
    return card->bucket < 4096;
}

int bfm_forest_impact_start(BfmForestImpact *impact, const BfmForestField *field,
                            const int16_t contact[3], const int16_t normal[3],
                            const int32_t eye[3])
{
    if (!impact || !field || !contact || !normal || !eye ||
        (!normal[0] && !normal[1] && !normal[2])) return 0;
    BfmForestImpact fresh = {0};
    const uint8_t *tilt = bfm_forest_room_bytes(field, UINT32_C(0x80189b48), 2);
    if (!tilt || !animation(&fresh.burst, field, UINT32_C(0x80189bac), UINT32_C(0x80189b6c), 8) ||
        !animation(&fresh.strip, field, UINT32_C(0x80189be8), UINT32_C(0x80189bb8), 6))
        return 0;
    int pitch, yaw;
    impact_face_normal(normal, &pitch, &yaw);
    bfm_forest_actor_pitch_yaw((unsigned)pitch, (unsigned)yaw, contact, &fresh.face);
    for (unsigned i = 0; i < 3; ++i)
        fresh.origin[i] = contact[i] + down(-2 * fresh.face.rotation[i][1] - 20 * fresh.face.rotation[i][2], 12);
    toward_eye(fresh.origin, eye, fresh.burst_origin);
    impact_strip_setup(fresh.strip_rotation, pitch, yaw, s16le(tilt), eye, fresh.origin);
    *impact = fresh;
    return 1;
}

void bfm_forest_impact_advance(BfmForestImpact *impact)
{
    if (impact && impact->cell < impact->burst.count) ++impact->cell;
}

static int burst_project(const int32_t origin[3], const BfmForestCamera *camera,
                         const uint8_t *d, BfmForestDustCard *card)
{
    int view[3];
    for (unsigned i = 0; i < 3; ++i) {
        int64_t dot = 0;
        for (unsigned j = 0; j < 3; ++j) dot += (int64_t)origin[j] * camera->rotation[i][j];
        int64_t axis = (int64_t)down(dot, 12) + camera->translation[i];
        if (axis < INT16_MIN || axis > INT16_MAX) return 0;
        view[i] = (int)axis;
    }
    if (view[2] <= 0 || view[2] / 4 + 1 >= 4096) return 0;
    int64_t q = ((int64_t)camera->projection * 65536 + view[2] / 2) / view[2];
    if (q > 131071) q = 131071;
    int scale = (int)(q / 8);
    int x = 160 + down(view[0] * q, 16) + down(s16le(d + 8) * scale, 12);
    int y = 120 + down(view[1] * q, 16) + down(s16le(d + 10) * scale, 12);
    int w = d[2] * scale / 4096, h = d[3] * scale / 4096;
    if (!w || !h) return 0;
    for (unsigned i = 0; i < 4; ++i) {
        card->vertices[i].x = x + (int)(i & 1) * w;
        card->vertices[i].y = y + (int)(i >> 1) * h;
    }
    card->bucket = (unsigned)view[2] / 4 + 1;
    return 1;
}

static void material(BfmForestDustCard *card, const BfmForestImpactAnimation *a,
                     unsigned cell, unsigned palette, int luminous)
{
    const uint8_t *c = a->cells + cell * 8;
    unsigned x = u16le(c), y = u16le(c + 2);
    unsigned u = (x % 64) * 4, v = y % 256;
    card->slot = 0;
    card->material = (BfmForestRasterMaterial){
        .page = (uint16_t)(x / 64 | (y / 256) << 4 | 1u << 5),
        .clut = (uint16_t)((256u + palette) << 6 | 352u / 16),
        .count = 4, .textured = 1, .translucent = luminous
    };
    for (unsigned i = 0; i < 4; ++i) {
        card->vertices[i].u = (uint8_t)(u + (i & 1) * (a->descriptor[2] - 1));
        card->vertices[i].v = (uint8_t)(v + (i >> 1) * (a->descriptor[3] - 1));
        card->vertices[i].rgb = luminous ? 0x606060 : 0x808080;
    }
}

unsigned bfm_forest_impact_project(const BfmForestImpact *impact, const BfmForestCamera *camera,
                                   BfmForestDustCard out[BFM_FOREST_IMPACT_CARDS])
{
    if (!impact || !camera || !camera->projection || !out || impact->cell >= impact->burst.count)
        return 0;
    unsigned count = 0;
    if (impact->cell < impact->strip.count) {
        const uint8_t *d = impact->strip.descriptor;
        if (impact_strip_project(impact->strip_rotation, impact->origin, camera,
                                 s16le(d + 8), s16le(d + 10), d[2], d[3], &out[count])) {
            material(&out[count++], &impact->strip, impact->cell, 23, 0);
        }
    }
    if (burst_project(impact->burst_origin, camera, impact->burst.descriptor, &out[count]))
        material(&out[count++], &impact->burst, impact->cell, 22, 1);
    return count;
}

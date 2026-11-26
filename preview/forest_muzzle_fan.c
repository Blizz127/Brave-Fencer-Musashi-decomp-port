/* SPDX-License-Identifier: MIT */
#include "forest_muzzle_fan.h"
#include "forest_opening_data.h"
#include "forest_shore_motion.h"
#include <math.h>
#include <string.h>

static int32_t down(int64_t value, int divisor)
{
    return (int32_t)(value / divisor - (value % divisor < 0));
}

/* Keep four base-four digits of the squared length. Each discarded digit
 * doubles the unit used to represent the distance. */
static unsigned magnitude(unsigned square, unsigned *scale)
{
    *scale = 3;
    if (square) {
        while (square > 255) { square >>= 2; ++*scale; }
        while (square < 64) { square <<= 2; --*scale; }
    }
    return square;
}

static void eye_direction(const int16_t muzzle[3], const int32_t eye[3], int dir[3])
{
    unsigned squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t d = (int64_t)eye[i] - muzzle[i];
        if (d < -16383 || d > 16383) {
            dir[0] = dir[2] = 0;
            dir[1] = -4095;
            return;
        }
        dir[i] = (int)d;
        squared += (unsigned)(d * d);
    }
    unsigned exponent, m = magnitude(squared, &exponent);
    int gain = m ? (int)sqrt((UINT32_C(1) << 30) / m) : 0;
    for (unsigned i = 0; i < 3; ++i)
        dir[i] = down((int64_t)dir[i] * gain, 1 << exponent);
}

/* Sample the actor's quarter wave with the sign of the original turn. */
static void turn(int angle, int *sine, int *cosine)
{
    const int16_t zero[3] = {0};
    BfmForestTransform rotation;
    bfm_forest_actor_root((unsigned)(angle < 0 ? -angle : angle), zero, &rotation);
    *sine = angle < 0 ? -rotation.rotation[0][2] : rotation.rotation[0][2];
    *cosine = rotation.rotation[0][0];
}

static void face(BfmForestMuzzlePlane *plane, int x, int y, int z)
{
    unsigned scale, m = magnitude((unsigned)((int64_t)x*x + (int64_t)z*z), &scale);
    unsigned length = ((unsigned)sqrt((double)m * 262144) << scale) / 4096;
    int pitch = (int)bfm_forest_shore_bearing(y, (int)length);
    int yaw = (int)bfm_forest_shore_bearing(x, z);
    int sp, cp, sy, cy;
    turn(-(pitch > 2048 ? pitch - 4096 : pitch), &sp, &cp);
    turn(yaw > 2048 ? yaw - 4096 : yaw, &sy, &cy);
    plane->axes[0][0] = cy;
    plane->axes[1][0] = 0;
    plane->axes[2][0] = -sy;
    plane->axes[0][1] = (int16_t)down(sy * sp, 4096);
    plane->axes[1][1] = cp;
    plane->axes[2][1] = (int16_t)down(cy * sp, 4096);
}

int bfm_forest_muzzle_fan_start(BfmForestMuzzleFan *fan, const BfmForestField *field,
                                const int16_t muzzle[3], const int32_t birth_eye[3])
{
    if (!fan || !field || !muzzle || !birth_eye) return 0;
    BfmForestMuzzleFan next = {.plane_count = 1};
    const uint8_t *rim = bfm_forest_room_bytes(field, UINT32_C(0x80199178), sizeof next.rim);
    if (!rim) return 0;
    memcpy(next.rim, rim, sizeof next.rim);
    int dir[3];
    eye_direction(muzzle, birth_eye, dir);
    for (unsigned i = 0; i < 3; ++i)
        next.planes[0].center[i] = muzzle[i] + down(dir[i], 128);
    face(&next.planes[0], dir[0], dir[1], dir[2]);
    BfmForestContact floor;
    if (bfm_forest_ground(field, muzzle[0], muzzle[1], muzzle[2], &floor) &&
        muzzle[1] - floor.height < 128) {
        BfmForestMuzzlePlane *p = &next.planes[next.plane_count++];
        p->center[0] = muzzle[0];
        p->center[1] = floor.height - 4;
        p->center[2] = muzzle[2];
        face(p, floor.normal[0], floor.normal[1], floor.normal[2]);
    }
    *fan = next;
    return 1;
}

typedef struct {
    BfmForestRasterVertex point;
    unsigned depth;
} FanPoint;

static FanPoint screen_point(const int64_t center[3], const int axes[3][2],
                             int x, int y, unsigned focal)
{
    int64_t v[3];
    for (unsigned i = 0; i < 3; ++i)
        v[i] = center[i] + down(x * axes[i][0] + y * axes[i][1], 4096);
    if (v[2] <= 0 || v[2] > UINT16_MAX || 2*v[2] <= focal ||
        v[0] < INT16_MIN || v[0] > INT16_MAX || v[1] < INT16_MIN || v[1] > INT16_MAX)
        return (FanPoint){0};
    int64_t ratio = ((int64_t)focal * 65536 + v[2] / 2) / v[2];
    int dx = down(v[0] * ratio, 65536), dy = down(v[1] * ratio, 65536);
    int x_screen = 160 + dx, y_screen = 120 + dy;
    if (x_screen < -1024 || x_screen > 1023 || y_screen < -1024 || y_screen > 1023)
        return (FanPoint){0};
    return (FanPoint){.point = {.x = x_screen, .y = y_screen}, .depth = (unsigned)v[2]};
}

unsigned bfm_forest_muzzle_fan_project(const BfmForestMuzzleFan *fan,
                                      const BfmForestCamera *camera, unsigned brightness,
                                      BfmForestDustCard out[BFM_FOREST_MUZZLE_FAN_CARDS])
{
    if (!fan || !camera || !camera->projection || !out || brightness == 0 || brightness > 255 ||
        fan->plane_count > 2) return 0;
    unsigned count = 0;
    for (unsigned bank = 0; bank < fan->plane_count; ++bank) {
        const BfmForestMuzzlePlane *p = &fan->planes[bank];
        int64_t center[3];
        int axes[3][2];
        for (unsigned i = 0; i < 3; ++i) {
            const int16_t *r = camera->rotation[i];
            int64_t t = 0;
            int64_t a = 0, b = 0;
            for (unsigned j = 0; j < 3; ++j) {
                t += (int64_t)r[j] * p->center[j];
                a += (int64_t)r[j] * p->axes[j][0];
                b += (int64_t)r[j] * p->axes[j][1];
            }
            center[i] = (int64_t)down(t, 4096) + camera->translation[i];
            axes[i][0] = down(a, 4096);
            axes[i][1] = down(b, 4096);
        }
        FanPoint hub = screen_point(center, axes, 0, 0, camera->projection), rim[9];
        if (!hub.depth) continue;
        hub.point.rgb = UINT32_C(0x010101) * brightness;
        for (unsigned i = 0; i < 9; ++i)
            rim[i] = screen_point(center, axes, fan->rim[i][0], fan->rim[i][1], camera->projection);
        for (unsigned i = 0; i < 8; i += 2) {
            if (!rim[i].depth || !rim[i+1].depth || !rim[i+2].depth) continue;
            unsigned bucket = rim[i+2].depth / 4;
            out[count++] = (BfmForestDustCard){
                .vertices = {rim[i].point, hub.point, rim[i+1].point, rim[i+2].point},
                .material = {.page = (2u << 7) | (1u << 5) | (640u / 64), .count = 4, .translucent = 1},
                .bucket = bucket < 4096 ? bucket : 4095};
        }
    }
    return count;
}

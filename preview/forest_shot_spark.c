/* SPDX-License-Identifier: MIT */
#include "forest_shot_spark.h"
#include "forest_shore_motion.h"

#include <math.h>

static int down(int value, int unit)
{
    return value / unit - (value % unit < 0);
}

static int signed_word(unsigned value)
{
    value &= 65535u;
    return value <= 32767u ? (int)value : (int)value - 65536;
}

static void wave(int angle, int *sine, int *cosine)
{
    const int16_t zero[3] = {0};
    BfmForestTransform turn;
    bfm_forest_actor_root((unsigned)(angle < 0 ? -angle : angle), zero, &turn);
    *sine = angle < 0 ? -turn.rotation[0][2] : turn.rotation[0][2];
    *cosine = turn.rotation[0][0];
}

/* The effect uses world heading, local pitch and local roll. Compose with
 * the signed field quarter wave and Q12 products. */
static void axes(int pitch, int yaw, int roll, BfmForestTransform *basis)
{
    int sx, cx, sy, cy, sz, cz;
    wave(pitch, &sx, &cx);
    wave(yaw, &sy, &cy);
    wave(roll, &sz, &cz);
    int a = down(sy * sx, 4096), b = down(cy * sx, 4096);
    *basis = (BfmForestTransform){.rotation={
        {down(cy*cz,4096) + down(a*sz,4096), down(a*cz,4096) - down(cy*sz,4096),
         down(sy*cx,4096)},
        {down(cx*sz,4096), down(cx*cz,4096), -sx},
        {down(b*sz,4096) - down(sy*cz,4096), down(b*cz,4096) + down(sy*sz,4096),
         down(cy*cx,4096)}
    }};
}

static int bearing(int a, int b)
{
    int angle = (int)bfm_forest_shore_bearing(a, b);
    return angle > 2048 ? angle - 4096 : angle;
}

/* Fix the orientation once at birth. Normalizing the signed direction before
 * its two bearings matters for short separations and shallow camera angles. */
static void face_eye(BfmForestTransform *face, const int16_t origin[3], const int32_t eye[3])
{
    int d[3] = {0};
    unsigned length2 = 0;
    for (unsigned k = 0; k < 3; ++k) {
        d[k] = signed_word((unsigned)eye[k] - (unsigned)origin[k]);
        if (d[k] < -16383 || d[k] > 16383) {
            d[0] = d[2] = 0;
            d[1] = -4095;
            length2 = 4095u * 4095u;
            break;
        }
        length2 += (unsigned)(d[k] * d[k]);
    }
    if (length2) {
        unsigned divisor = 8;
        while (length2 > 255) { length2 >>= 2; divisor *= 2; }
        while (length2 < 64) { length2 <<= 2; divisor /= 2; }
        int gain = (int)sqrt((double)(1u << 30) / length2);
        for (unsigned k = 0; k < 3; ++k) d[k] = down(d[k] * gain, (int)divisor);
    }
    axes(-bearing(d[1], (int)hypot(d[0], d[2])), bearing(d[0], d[2]), 0, face);
}

int bfm_forest_shot_spark_fire(BfmForestShotSpark *spark, const int16_t origin[3],
                  const int16_t direction_angles[3], unsigned variant,
                  unsigned random, const int32_t eye[3])
{
    if (!spark || !origin || !direction_angles || !eye || (variant != 2 && variant != 3))
        return 0;
    BfmForestShotSpark spawn = {.step = 1, .variant = variant};
    BfmForestTransform normal, scatter;
    axes(direction_angles[0], direction_angles[1], direction_angles[2], &normal);
    axes(3648 + (int)(random & 127) * 8, 3584 + (int)((random >> 8) & 127) * 8,
         0, &scatter);
    face_eye(&spawn.face, origin, eye);
    for (unsigned k = 0; k < 3; ++k) {
        int axis = 0;
        for (unsigned j = 0; j < 3; ++j)
            axis += normal.rotation[k][j] * scatter.rotation[j][2];
        spawn.velocity[k] = (int16_t)down(down(axis, 4096) * 10, 4096);
        spawn.position[k] = origin[k];
    }
    *spark = spawn;
    return 1;
}

void bfm_forest_shot_spark_tick(BfmForestShotSpark *spark)
{
    if (!spark || !spark->step) return;
    if (spark->step >= 11) {
        spark->step = 0;
        return;
    }
    for (unsigned k = 0; k < 3; ++k) {
        spark->face.translation[k] = spark->position[k];
        spark->position[k] = (int16_t)signed_word((unsigned)(spark->position[k] + spark->velocity[k]));
    }
    ++spark->velocity[1];
    ++spark->step;
}

size_t bfm_forest_shot_spark_project(const BfmForestShotSpark *spark,
                  const BfmForestCamera *camera, BfmForestDustCard *cards, size_t capacity)
{
    if (!spark || spark->step < 2 || spark->step > 11 || !cards || !capacity) return 0;
    unsigned age = spark->step - 2;
    int scale = (spark->variant == 3 ? 1228 : 2048) + 128 * (int)age;
    BfmForestTransform face = spark->face;
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j)
            face.rotation[i][j] = (int16_t)down(face.rotation[i][j] * scale, 4096);
    BfmForestCamera local;
    if (!bfm_forest_effect_camera(camera, &face, &local)) return 0;

    /* A square and a diamond share the same four Gouraud colors. The
     * original gradient leaves one component per vertex unspecified.
     * Give these components zero intensity in the native material. */
    unsigned bright = 192 - age * 16;
    BfmForestDustCard card = {.material={.count=4, .translucent=1, .page=32}};
    card.vertices[0].rgb = card.vertices[1].rgb = 32 | (bright << 8);
    card.vertices[2].rgb = card.vertices[3].rgb = (32 << 8) | (32 << 16);
    size_t count = 0;
    for (unsigned shape = 0; shape < BFM_FOREST_SHOT_SPARK_CARDS && count < capacity; ++shape) {
        int16_t corners[4][3] = {{0}};
        for (unsigned v = 0; v < 4; ++v) {
            int x = (v & 2) ? 1 : -1, y = (v & 1) ? 1 : -1;
            corners[v][0] = (int16_t)(shape ? (x+y)*7/2 : x*5);
            corners[v][1] = (int16_t)(shape ? (y-x)*7/2 : y*5);
        }
        if (bfm_forest_project_effect_quad(&local, corners, &card)) cards[count++] = card;
    }
    return count;
}

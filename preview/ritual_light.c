/* SPDX-License-Identifier: MIT */
#include "ritual_light.h"
#include <limits.h>
#include <math.h>
#include <string.h>

/* Twelve equal turns on a 4096-unit circle put the positive quadrant at
   angles 0, 342, 683 and 1024 after rounding each intermediate turn up.
   These are their signed-unit trigonometric magnitudes in 12-bit fixed. */
static void rim_point(int point[3], unsigned index, int radius,
                      int x, int y, int depth)
{
    static const int cosine[4] = {4096, 3545, 2046, 0};
    static const int sine[4]   = {0, 2052, 3548, 4096};
    unsigned quadrant = index / 3, step = index % 3;
    unsigned acute = quadrant & 1u ? 3u - step : step;
    int dx = radius * cosine[acute] / 4096;
    int dy = radius * sine[acute] / 4096;
    dx *= 256;
    dy *= 256;
    /* Negative offsets use floor division after projection. The local
       magnitudes have already been reduced to integer world units. */
    point[0] = x + ((quadrant == 1 || quadrant == 2)
                   ? -(dx / depth) - (dx % depth != 0) : dx / depth);
    point[1] = y + (quadrant >= 2
                   ? -(dy / depth) - (dy % depth != 0) : dy / depth);
    point[2] = depth;
}

static void emit_halo(int screen_x, int screen_y, int camera_z, unsigned age,
                          BfmRitualEmit emit, void *context)
{
    /* Even at unit depth the projected radius is bounded by 32768. */
    if (!emit || camera_z <= 0 || screen_x < INT_MIN + 32768 ||
        screen_x > INT_MAX - 32768 || screen_y < INT_MIN + 32768 ||
        screen_y > INT_MAX - 32768)
        return;

    const unsigned rgb[4][3] = {{0,0,0}, {0,0,0}, {0,224,136}, {0,0,0}};
    int radius = age < 28 ? 128 - 4 * (int)age : 16;
    int ring[12][3];
    for (unsigned i = 0; i < 12; ++i)
        rim_point(ring[i], i, radius, screen_x, screen_y, camera_z);

    for (unsigned segment = 0; segment < 6; ++segment) {
        unsigned start = (3 + 2 * segment) % 12;
        int xyz[4][3];
        memcpy(xyz[0], ring[(start + 2) % 12], sizeof(xyz[0]));
        memcpy(xyz[1], ring[(start + 1) % 12], sizeof(xyz[1]));
        xyz[2][0] = screen_x;
        xyz[2][1] = screen_y;
        xyz[2][2] = camera_z;
        memcpy(xyz[3], ring[start], sizeof(xyz[3]));
        emit(context, 4, xyz, rgb, camera_z / 4 + 1);
    }
}

int bfm_ritual_light_load(BfmRitualNodes *out, const uint8_t *scene, size_t bytes)
{
    const size_t material = 0x6097c, first = 0x60a1c, count = 12, stride = 8;
    if (!out || !scene || bytes < first + count * stride)
        return 0;
    BfmRitualNodes parsed;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t *entry = scene + first + stride * i;
        if (entry[6] || entry[7])
            return 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            unsigned raw = entry[axis * 2] | (unsigned)entry[axis * 2 + 1] << 8;
            int value = (int)raw;
            if (raw >= 32768)
                value -= 65536;
            parsed.point[i][axis] = (int16_t)value;
        }
    }
    for (unsigned ch = 0; ch < 3; ++ch)
        parsed.tint[ch] = scene[material + ch];
    *out = parsed;
    return 1;
}

static int64_t floor_by(int64_t value, int divisor)
{
    return value / divisor - (value < 0 && value % divisor != 0);
}

/* The projection scale is the 16-bit fractional reciprocal of camera depth
   with focal length 256. Normalize the denominator into [0x8000,0xffff],
   seed the reciprocal from its high bits and refine at fixed precision. */
static unsigned projection_scale(unsigned z)
{
    if (z <= 128)
        return 0x1ffff;
    unsigned d = z, numerator = 256;
    while (d < 0x8000u) {
        d *= 2;
        numerator *= 2;
    }
    unsigned index = (d - 0x7fc0u) / 128u;
    unsigned seed = (0x40000u / (256u + index) + 1u) / 2u;
    if (seed < 257u)
        seed = 257u;
    unsigned correction = (0x2000080u - d * seed) / 256u;
    unsigned refined = (128u + correction * seed) / 256u;
    return (unsigned)(((uint64_t)numerator * refined + 32768u) / 65536u);
}

static unsigned halo_start(unsigned i)
{
    return 4404u + (i < 4 ? 18u * i : 72u + 4u * (i - 4u));
}

/* Each new halo advances the physical crystal by four extra angle units
 * per draw tick. The extra rotation persists after the halo fades. */
unsigned bfm_ritual_extra_yaw(unsigned field)
{
    unsigned extra = 0;
    for (unsigned i = 0; i < 12; ++i) {
        unsigned start = halo_start(i);
        if (field < start) break;
        extra += 4u * (1u + (field - start) / 2u);
    }
    return extra & 4095u;
}

static void prepare_halos(const BfmRitualNodes *nodes, unsigned field,
                           const int axes[3][3], const int origin[3],
                           BfmRitualEmit emit, void *context)
{
    if (!nodes || !axes || !origin || !emit || field < 4404u || field >= 4632u)
        return;
    /* When depth ties, the newer node precedes the older one on the
       transparent draw list. The caller merges with the court's buckets. */
    for (unsigned n = 12; n > 0; --n) {
        unsigned i = n - 1;
        unsigned start = halo_start(i);
        if (field < start || (i == 0 && field >= 4570u))
            continue;
        int64_t camera[3];
        for (unsigned j = 0; j < 3; ++j) {
            int64_t sum = 0;
            for (unsigned k = 0; k < 3; ++k)
                sum += (int64_t)axes[j][k] * nodes->point[i][k];
            camera[j] = floor_by(sum, 4096) + origin[j];
        }
        if (camera[2] <= 128 || camera[2] > 65535)
            continue;
        unsigned scale = projection_scale((unsigned)camera[2]);
        int64_t x = 160 + floor_by(camera[0] * scale, 65536);
        int64_t y = 120 + floor_by(camera[1] * scale, 65536);
        if (x < -1024 || x > 1023 || y < -1024 || y > 1023)
            continue;
        emit_halo((int)x, (int)y, (int)camera[2], (field - start) / 2,
                            emit, context);
    }
}

static int floor_div(int64_t value, int divisor)
{
    int result = (int)(value / divisor);
    return result - (value % divisor < 0);
}

static void project(const int axes[3][3], const int translation[3],
                    const int point[3], int projected[3])
{
    int64_t view[3];
    for (int row = 0; row < 3; ++row) {
        int64_t value = 0;
        for (int column = 0; column < 3; ++column)
            value += (int64_t)axes[row][column] * point[column];
        view[row] = translation[row] + floor_by(value, 4096);
    }
    int64_t scale = view[2] > 0 ? (256 * 65536 + view[2] / 2) / view[2] : 131071;
    if (scale > 131071) scale = 131071;
    for (unsigned axis = 0; axis < 2; ++axis) {
        int64_t screen = floor_by(view[axis] * scale, 65536);
        if (screen < -1024) screen = -1024;
        if (screen > 1023) screen = 1023;
        projected[axis] = (axis ? 120 : 160) + (int)screen;
    }
    projected[2] = (int)view[2];
}

static void prepare_rays(unsigned opening_field, const int axes[3][3],
                           const int translation[3], BfmRitualEmit emit,
                           void *context)
{
    enum { FIRST_FIELD = 4290, INTERVAL = 8, BATCHES = 14, ACTIVE_BATCHES = 5, MAX_AGE = 39 };
    if (opening_field < FIRST_FIELD ||
        opening_field >= FIRST_FIELD + 2 * ((BATCHES - 1) * INTERVAL + MAX_AGE)) return;
    int tick = (int)((opening_field - FIRST_FIELD) / 2);
    static const unsigned colors[4][3] = {{0, 224, 136}, {64, 96, 16}};
    /* Sixteen spokes share the XY wheel. Successive eight-tick bursts
     * alternate its depth sequence and the length of each streak. */
    int latest = tick / INTERVAL;
    if (latest >= BATCHES) latest = BATCHES - 1;
    /* Five batches can overlap. For an equal ordering depth the original
     * submits the higher occupied batch index first, then increasing slots. */
    for (int index = ACTIVE_BATCHES - 1; index >= 0; --index) {
        int batch = latest - (latest + ACTIVE_BATCHES - index) % ACTIVE_BATCHES;
        if (batch < 0) continue;
        int age = tick - batch * INTERVAL;
        for (int slot = 0; slot < 16; ++slot) {
            int spoke = (9 * slot) % 16;
            double angle = (22.0 * 3.14159265358979323846 / 180.0) * spoke;
            int x = (int)(384.0 * cos(angle));
            int y = (int)(384.0 * sin(angle));
            int span = x < 0 ? -x : x;
            int magnitude_y = y < 0 ? -y : y;
            if (magnitude_y > span) span = magnitude_y;
            int denominator = 4 * span;
            if (age * (age + 1) >= denominator) continue;
            int rank = (int)((unsigned)(9 - 7 * slot + 16 * batch) % 32u);
            const int origin[3] = {x, y, 8 * rank};
            int xyz[4][3] = {{0}};
            /* Four times the distance travelled along the dominant world axis.
             * Keeping the normalized direction in fixed point also preserves
             * the one-pixel difference near the end of a shallow spoke. */
            int travel = age * (age - 1);
            for (int end = 0; end < 2; ++end) {
                int point[3];
                for (int axis = 0; axis < 3; ++axis) {
                    int direction = origin[axis] * 32768 / span;
                    point[axis] = origin[axis] + floor_div(-direction * travel, 4 * 32768);
                }
                point[1] -= 779;
                project(axes, translation, point, xyz[end]);
                travel -= 4 * ((batch & 1) ? 68 : 39);
                if (travel < 0) travel = 0;
            }
            emit(context, 2, xyz, colors, (xyz[0][2] + xyz[1][2]) / 8);
        }
    }
}

void bfm_ritual_light_prepare(const BfmRitualNodes *nodes, unsigned field,
                              const int view[3][3], const int origin[3],
                              BfmRitualEmit emit, void *context)
{
    if (!nodes || !view || !origin || !emit) return;
    /* Newer effects precede older batches in the same ordering bucket. */
    bfm_ritual_rings(field, view, origin, nodes->tint, emit, context);
    prepare_halos(nodes, field, view, origin, emit, context);
    prepare_rays(field, view, origin, emit, context);
}

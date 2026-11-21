#include "geometry.h"

#include <assert.h>
#include <stdint.h>

typedef s32 (*Predicate)(const ScreenPoint *, const ScreenPoint *, const SpatialPoint *);

static unsigned calls;
static u32 arguments[3];
static s32 callback_result;

s32 func_800495EC(u32 first, u32 second, u32 third)
{
    ++calls;
    arguments[0] = first;
    arguments[1] = second;
    arguments[2] = third;
    return callback_result;
}

static u32 xy(s16 x, s16 y)
{
    return (u32)(u16)x | ((u32)(u16)y << 16);
}

static void check(Predicate fn, ScreenPoint a, ScreenPoint b, s16 x, s16 z,
                  s32 result, unsigned expected_calls, s32 expected_answer)
{
    /* Vary the unused component independently of both packed coordinates. */
    const s16 unused[] = {INT16_MIN, INT16_MAX};
    for (unsigned i = 0; i < 2; ++i) {
        SpatialPoint point = {x, unused[i], z};
        calls = 0;
        callback_result = result;
        assert(fn(&a, &b, &point) == expected_answer);
        assert(calls == expected_calls);
        if (expected_calls) {
            assert(arguments[0] == xy(a.x, a.y));
            assert(arguments[1] == xy(b.x, b.y));
            assert(arguments[2] == xy(x, z));
        }
    }
}

int main(void)
{
    const ScreenPoint a = {-7, 9}, b = {5, -3};
    const ScreenPoint c = {5, 9}, d = {-7, -3};
    /* The first and second expected results exercise the two orientations. */
    const struct { s16 x, z; s32 first, second; } outside[] = {
        {-8, 15, 0, 0}, {6, -8, 1, 1}, {0, -4, 0, 1}, {0, 10, 1, 0}
    };
    for (unsigned i = 0; i < sizeof outside / sizeof outside[0]; ++i) {
        check(func_8017D5F4, a, b, outside[i].x, outside[i].z, 0, 0, outside[i].first);
        check(func_8017D6AC, c, d, outside[i].x, outside[i].z, 0, 0, outside[i].second);
    }

    const struct { s16 x, z; s32 reply, expected; } inside[] = {
        {-7, 4, INT32_MIN, 0}, {5, 0, INT32_MAX, 1},
        {0, -3, -1, 0}, {1, 9, 0, 1}, {-2, 6, 1, 1}
    };
    for (unsigned i = 0; i < sizeof inside / sizeof inside[0]; ++i) {
        check(func_8017D5F4, a, b, inside[i].x, inside[i].z, inside[i].reply, 1, inside[i].expected);
        check(func_8017D6AC, c, d, inside[i].x, inside[i].z, inside[i].reply, 1, inside[i].expected);
    }
    check(func_8017D5F4, a, a, a.x, a.y, 0, 1, 1);
    check(func_8017D6AC, a, a, a.x, a.y, 0, 1, 1);

    /* At these corners, +65535 squared wraps to -131071 in the
       signed 32-bit callback result, and its negative wraps to +131071. */
    const ScreenPoint left = {INT16_MIN, INT16_MAX}, right = {INT16_MAX, INT16_MIN};
    const ScreenPoint top = {INT16_MAX, INT16_MAX}, bottom = {INT16_MIN, INT16_MIN};
    check(func_8017D5F4, left, right, INT16_MAX, INT16_MAX, -131071, 1, 0);
    check(func_8017D5F4, left, right, INT16_MIN, INT16_MIN, 131071, 1, 1);
    check(func_8017D6AC, top, bottom, INT16_MAX, INT16_MIN, -131071, 1, 0);
    check(func_8017D6AC, top, bottom, INT16_MIN, INT16_MAX, 131071, 1, 1);
    return 0;
}

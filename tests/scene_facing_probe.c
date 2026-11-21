#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include "geometry.h"

static s32 area_result;
static int area_calls;
static int bearing_calls;
static s32 bearing_x, bearing_z;

s32 func_800495EC(u32 first, u32 second, u32 third)
{
    (void)first; (void)second; (void)third;
    ++area_calls;
    return area_result;
}

s32 func_8004CFEC(s32 dx, s32 dz)
{
    ++bearing_calls;
    bearing_x = dx;
    bearing_z = dz;
    return -5;
}

int main(void)
{
    FacingRegion records[] = {
        {0, 0, {-7, 9}, {8, -5}, {SHRT_MIN, SHRT_MAX}},
        {1, 1, {8, 9}, {-7, -5}, {-3, 6}},
        {255, 2, {0, 0}, {0, 0}, {0, 0}}
    };
    SpatialPoint position = {0, 123, 0};
    SpatialPoint next = {SHRT_MAX, -456, SHRT_MIN};
    assert(sizeof(FacingRegion) == 14 && offsetof(FacingRegion, origin) == 10);
    assert(sizeof(SpatialPoint) == 6);
    area_result = -1;
    assert(func_8017D484(records, &position, &next, 7) == 4091);
    assert(area_calls == 1 && bearing_calls == 1);
    assert(bearing_x == 65535 && bearing_z == -65535);

    area_calls = bearing_calls = 0;
    area_result = 0;
    assert(func_8017D484(records, &position, &next, 0) == 2048);
    assert(area_calls == 2 && bearing_calls == 0);

    records[0].side = 3;
    area_calls = bearing_calls = 0;
    area_result = -11;
    assert(func_8017D484(records, &position, &next, -1) == 4091);
    assert(area_calls == 1 && bearing_calls == 1);
    assert(bearing_x == -32770 && bearing_z == 32774);
    area_calls = bearing_calls = 0;
    assert(func_8017D484(records, &position, &next, 0) == 4091);
    assert(area_calls == 0 && bearing_calls == 1);
    assert(bearing_x == 65535 && bearing_z == -65535);

    records[0].side = 255;
    records[0].facing = 7;
    records[1].side = 17;
    records[1].facing = 2;
    area_calls = bearing_calls = 0;
    assert(func_8017D484(records, &position, &next, 1) == 2048);
    assert(area_calls == 0 && bearing_calls == 0);
    return 0;
}

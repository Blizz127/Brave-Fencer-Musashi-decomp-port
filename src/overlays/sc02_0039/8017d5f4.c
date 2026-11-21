#include "geometry.h"

s32 func_8017D5F4(const ScreenPoint *a, const ScreenPoint *b, const SpatialPoint *p)
{
    PackedPoint first, second, third;

    if (p->x < a->x) return 0;
    if (p->x > b->x) return 1;
    if (p->z < b->y) return 0;
    if (p->z > a->y) return 1;

    first.point.x = a->x;
    first.point.y = a->y;
    second.point.x = b->x;
    second.point.y = b->y;
    third.point.x = p->x;
    third.point.y = p->z;
    if (func_800495EC(SCREEN_POINT_WORD(first), SCREEN_POINT_WORD(second),
                      SCREEN_POINT_WORD(third)) < 0) return 0;
    return 1;
}

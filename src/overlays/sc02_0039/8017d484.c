#include "geometry.h"

extern s32 func_8004CFEC(s32 dx, s32 dz);

s32 func_8017D484(const FacingRegion *region, const SpatialPoint *position,
                  const SpatialPoint *next, s32 skip)
{
    for (;;) {
        switch (region->side) {
        case 0:
            skip = func_8017D5F4(&region->first, &region->second, position);
            break;
        case 1:
            skip = func_8017D6AC(&region->first, &region->second, position);
            break;
        case 255:
            skip = 0;
            break;
        }
        if (!skip) {
            switch (region->facing) {
            case 0:
                return func_8004CFEC(next->x - region->origin.x, next->z - region->origin.y) & 0xfff;
            case 1:
                return func_8004CFEC(region->origin.x - next->x, region->origin.y - next->z) & 0xfff;
            case 2:
                return 0x800;
            }
        }
        ++region;
    }
}

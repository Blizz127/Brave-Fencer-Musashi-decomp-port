/* Link the box classifier to all three face routines and exercise their shared state. */
#include <assert.h>
#include <stddef.h>
#include "forest_floor.h"

static FloorPoint near_point, far_point, contact, movement;
FloorPoint *D_801870AC = &near_point, *D_801870B0 = &far_point;
FloorPoint *D_801870B4 = &contact, *D_801870B8 = &movement;
FloorPoint D_801152A8, D_80126720;
u32 D_801150D8;

static FloorBox box = {0, -10, 10, -12, 18, -8, 14};

void func_80047E98(FloorPoint *input, FloorPoint *output)
{
    (void)input;
    (void)output;
    assert(0 && "outside-to-inside crossings do not call the inside normalizer");
}

static void reset(FloorPoint start, FloorPoint end, FloorPoint step)
{
    near_point = start;
    far_point = end;
    movement = step;
    contact = (FloorPoint){53, 54, 55, 56};
    D_801152A8 = (FloorPoint){41, 42, 43, 44};
    D_80126720 = (FloorPoint){61, 62, 63, 64};
    D_801870B4 = &contact;
    D_801150D8 = 0x18;
}

static void xyz(const FloorPoint *point, s16 x, s16 y, s16 z)
{
    assert((s16)point->x == x && (s16)point->y == y && (s16)point->z == z);
}

int main(void)
{
    FloorPoint origin = {0};

    /* At the corner entry, the Z face accepts the inclusive X low bound. */
    reset((FloorPoint){-20, 2, -16, 0}, (FloorPoint){0, 6, 0, 0}, origin);
    assert(func_80135EB0(&box, 0) == 1);
    xyz(&movement, 20, 4, 16);
    xyz(&contact, -10, 4, -10);
    xyz(&D_801152A8, 0, 0, -4095);
    xyz(&D_80126720, 0, 3, 3);
    assert(contact.w == 56 && D_80126720.w == 64 && D_801150D8 == 0x18);

    /* The low Z projection misses X. The low X face then accepts the crossing. */
    reset((FloorPoint){-20, 2, -9, 0}, (FloorPoint){0, 6, 1, 0}, origin);
    assert(func_80135EB0(&box, 0) == 1);
    xyz(&contact, -12, 4, -4);
    xyz(&D_801152A8, -4095, 0, 0);

    /* Bit 15 replaces the in-plane coordinates after the projection passes. */
    reset((FloorPoint){-20, 2, 0, 0}, (FloorPoint){0, 10, 8, 0}, origin);
    assert(func_80135EB0(&box, 0x8000) == 1);
    xyz(&contact, -12, 10, 8);
    xyz(&D_801152A8, -4095, 0, 0);
    reset((FloorPoint){3, 24, 4, 0}, (FloorPoint){5, 12, 10, 0}, origin);
    assert(func_80135EB0(&box, 0x8000) == 1);
    xyz(&contact, 5, 20, 10);
    xyz(&D_801152A8, 0, 4095, 0);

    /* A rejected Y face retains its projection, even with a far-point override. */
    reset((FloorPoint){15, 24, 4, 0}, origin, (FloorPoint){2, -12, 6, 0});
    assert(func_80136824(&box, 0x8001, 18) == 0);
    xyz(&contact, 16, 18, 7);
    xyz(&D_801152A8, 41, 42, 43);
    xyz(&D_80126720, 61, 62, 63);

    /* The sign branches write the face contact and Z normal in opposite order. */
    for (s32 high = 0; high <= 1; ++high) {
        reset(origin, origin, (FloorPoint){0, 0, 10, 0});
        D_801870B4 = &D_801152A8;
        assert(func_80136334(&box, high, high ? 14 : -8) == 1);
        xyz(&D_801152A8, 0, 0, high ? 4095 : -10);
        assert(D_801152A8.w == 44);
    }
    /* A contact at box + 6 replaces xmax before the midpoint reads it. */
    {
        union {
            FloorBox bounds;
            struct { s16 prefix[3]; FloorPoint point; } shifted;
        } overlapping = {.bounds = {0, -125, 126, -269, 270, -303, 304}};
        assert(offsetof(struct { s16 prefix[3]; FloorPoint point; }, point) == 6);
        reset((FloorPoint){2, -6, 8, 0}, origin, (FloorPoint){7, 9, -11, 0});
        D_801870B4 = &overlapping.shifted.point;
        assert(func_801365B8(&overlapping.bounds, 0, 7) == 1);
        assert(overlapping.bounds.xhi == 5 && overlapping.bounds.ylo == 0 && overlapping.bounds.yhi == 1);
        xyz(&D_801152A8, -4095, 0, 0);
        xyz(&D_80126720, -60, 0, 0);
    }
    return 0;
}

#include <assert.h>
#include <stddef.h>
#include "forest_floor.h"

static FloorPoint near_point, far_point, hit, direction;
FloorPoint *D_801870AC = &near_point, *D_801870B0 = &far_point;
FloorPoint *D_801870B4 = &hit, *D_801870B8 = &direction;
FloorPoint D_801152A8, D_80126720;
u32 D_801150D8;

static FloorBox bounds = {0, -10, 10, -20, 10, -12, 16};
static int calls, normal_calls, succeeds_on;
static struct { char axis; s32 flags; s16 boundary; } seen[3];

static s32 face(FloorBox *box, char axis, s32 flags, s16 boundary)
{
    assert(box == &bounds && calls < 3);
    seen[calls].axis = axis;
    seen[calls].flags = flags;
    seen[calls].boundary = boundary;
    /* Failed calls can write a projection before the next face is tried. */
    hit.w = (s16)(hit.w + 1);
    return ++calls == succeeds_on;
}

s32 func_80136334(FloorBox *box, s32 flags, s16 boundary)
{
    return face(box, 'z', flags, boundary);
}
s32 func_801365B8(FloorBox *box, s32 flags, s16 boundary)
{
    return face(box, 'x', flags, boundary);
}
s32 func_80136824(FloorBox *box, s32 flags, s16 boundary)
{
    return face(box, 'y', flags, boundary);
}

void func_80047E98(FloorPoint *input, FloorPoint *output)
{
    assert(input == &D_801152A8 && output == input);
    assert((s16)input->x == 2 && input->y == 0 && (s16)input->z == 3);
    assert((s16)D_80126720.x == 0 && (s16)D_80126720.y == -5 && (s16)D_80126720.z == 2);
    assert(hit.x == 2 && (s16)hit.y == -2 && hit.z == 5 && hit.w == 77);
    ++normal_calls;
    output->x = 81;
}

static void reset(FloorPoint start, FloorPoint end)
{
    near_point = start;
    far_point = end;
    hit = (FloorPoint){77,77,77,77};
    direction = (FloorPoint){88,88,88,88};
    D_801152A8 = (FloorPoint){90,91,92,93};
    D_80126720 = (FloorPoint){101,102,103,104};
    D_801150D8 = 0x124;
    normal_calls = calls = succeeds_on = 0;
}

int main(void)
{
    FloorPoint inside = {2,(u16)-2,5,11}, origin = {0,0,0,12};
    assert(sizeof bounds == 16 && offsetof(FloorBox, zhi) == 14);
    reset(inside, origin);
    assert(func_80135EB0(&bounds, 0) == 1 && normal_calls == 1 && !calls);
    assert(D_801150D8 == 0x125 && D_801152A8.x == 81 && D_801152A8.w == 93);
    assert(direction.x == 88 && direction.w == 88 && D_80126720.w == 104);

    reset((FloorPoint){(u16)-20,3,5,0}, (FloorPoint){(u16)-19,0,5,0});
    assert(!func_80135EB0(&bounds, 2) && !normal_calls && !calls);
    assert(direction.x == 88 && hit.w == 77 && D_801150D8 == 0x124);

    for (int accepted = 0; accepted <= 3; ++accepted) {
        reset((FloorPoint){(u16)-20,30,(u16)-25,0}, origin);
        succeeds_on = accepted;
        assert(func_80135EB0(&bounds, (s32)0x12348002u) == (accepted != 0));
        assert(calls == (accepted ? accepted : 3) && !normal_calls);
        assert(seen[0].axis == 'z' && seen[0].flags == (s16)0x8002 && seen[0].boundary == -12);
        if (calls >= 2)
            assert(seen[1].axis == 'x' && seen[1].flags == (s16)0x8002 && seen[1].boundary == -10);
        if (calls == 3)
            assert(seen[2].axis == 'y' && seen[2].flags == (s16)0x8003 && seen[2].boundary == 10);
        assert(direction.x == 20 && (s16)direction.y == -30 && direction.z == 25);
        assert(direction.w == 88 && hit.w == 77 + calls && D_801150D8 == 0x124);
    }

    reset((FloorPoint){20,0,25,0}, origin);
    assert(!func_80135EB0(&bounds, 4) && calls == 2);
    assert(seen[0].axis == 'x' && seen[0].flags == 5 && seen[0].boundary == 10);
    assert(seen[1].axis == 'z' && seen[1].flags == 5 && seen[1].boundary == 16);

    {
        /* The first direction store changes the near Y read by the next subtraction. */
        union {
            struct { FloorPoint point; u16 extra; } base;
            struct { u16 extra; FloorPoint point; } shifted;
        } points;
        reset((FloorPoint){(u16)-20,0,(u16)-20,7}, (FloorPoint){0,0,30,8});
        points.base.point = near_point;
        points.base.extra = 0;
        D_801870AC = &points.base.point;
        D_801870B8 = &points.shifted.point;
        succeeds_on = 1;
        assert(func_80135EB0(&bounds, 0) && calls == 1);
        assert(D_801870B8->x == 20 && (s16)D_801870B8->y == -20 && D_801870B8->z == 50);
        assert((s16)D_801870AC->y == 20 && (s16)D_801870AC->z == -20);
    }
    return 0;
}

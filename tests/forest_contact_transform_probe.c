#include "forest_floor.h"
#include <assert.h>
#include <stddef.h>

FloorPoint D_801152A8, D_80126720, *D_801870B4;
static FloorPoint contact, alternate;
static int selected_matrix, calls, rebind;

/* Test callbacks verify the original in-place arguments and call sequence. */
void func_80048FBC(void *matrix, FloorPoint *input, FloorPoint *output)
{
    assert(matrix == &selected_matrix && input == output);
    assert(input == (calls == 0 ? &contact : &D_80126720));
    assert(calls < 2);
    ++calls;
    u16 x = input->x;
    output->x = input->z;
    output->z = (u16)(0u - x);
}

void func_8004864C(FloorPoint *input, s32 *output)
{
    assert(calls == 2 && input == &D_801152A8);
    assert(input->x == 32 && input->y == 29 && input->z == 19);
    ++calls;
    output[0] = 0x123456;
    output[1] = -0x123456;
    output[2] = 0x10000;
    if (rebind) D_801870B4 = &alternate;
}

static void same(FloorPoint p, u16 x, u16 y, u16 z, s16 w)
{
    assert(p.x == x && p.y == y && p.z == z && p.w == w);
}

static void reset(void)
{
    contact = (FloorPoint){5, 20, 11, 98};
    D_80126720 = (FloorPoint){41, 42, 43, 62};
    D_801152A8 = (FloorPoint){32, 29, 19, 63};
    alternate = (FloorPoint){300, 400, 500, 73};
    D_801870B4 = &contact;
    calls = rebind = 0;
}

int main(void)
{
    _Static_assert(offsetof(FloorPoint, y) == 2 && offsetof(FloorPoint, z) == 4, "component offsets");
    _Static_assert(offsetof(FloorContactRecord, x) == 0x48 &&
                   offsetof(FloorContactRecord, y) == 0x4c &&
                   offsetof(FloorContactRecord, z) == 0x50, "translation offsets");
    FloorContactRecord record = {.x = 0x7fffffff, .y = 0xffffffff, .z = 0x80000000};
    FloorPoint result = {0, 0, 0, 42};
    reset();
    func_80136A94(0, &record, &result, NULL);
    same(result, 4, 19, 11, 42);
    same(D_80126720, 40, 41, 43, 62);
    same(D_801152A8, 32, 29, 19, 63);
    same(contact, 5, 20, 11, 98);
    assert(calls == 0);

    record = (FloorContactRecord){.x = 7, .y = 9, .z = 13};
    reset();
    func_80136A94(1, &record, &contact, &selected_matrix);
    assert(calls == 3);
    same(contact, 18, 29, 8, 98);
    same(D_80126720, 50, 51, (u16)-28, 62);
    same(D_801152A8, 0x3456, 0xcbaa, 0, 63);

    reset();
    rebind = 1;
    func_80136A94(-1, &record, &D_80126720, &selected_matrix);
    assert(calls == 3);
    same(contact, 11, 20, (u16)-5, 98);
    same(alternate, 300, 400, 500, 73);
    /* The output aliases the auxiliary point, so both translations apply. */
    same(D_80126720, 314, 418, 526, 62);
    same(D_801152A8, 0x3456, 0xcbaa, 0, 63);
    return 0;
}

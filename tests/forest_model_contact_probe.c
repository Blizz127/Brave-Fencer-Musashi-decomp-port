/* The model list stores game addresses even when the host uses 64-bit pointers. */
#include <assert.h>
#include <setjmp.h>
#include "forest_floor.h"

enum { FIRST = 0x80100100u, SECOND = 0x80100110u, MISSING = 0x80100200u };
static FloorBox first, second;
static FloorGrid grid;
static FloorPoint near_point, far_point, projected;
static FloorContactRecord record;
static FloorPoint start, end;
static int kind, box_calls, grid_calls, transforms, retry_limit;
static int box_lookups, grid_lookups, fail_grid_lookup, change_x_on_lookup;
static u32 next_after_call = SECOND;
static u32 rejected;
static jmp_buf unavailable;
FloorPoint *D_801870AC = &near_point, *D_801870B0 = &far_point;
FloorPoint *D_801870B4 = &projected;
FloorPoint D_801152A8, D_801152B0;
FloorBox D_801D9524;
FloorMatrix D_801D9504;
s16 D_801D94FC;
u16 D_801D9500;

void *bfm_forest_contact_model(u32 address, s32 is_box)
{
    assert(address != 0);
    if (!is_box) {
        ++grid_lookups;
        assert(D_801D9500 == 0 && D_801D94FC == 0);
        if (change_x_on_lookup)
            near_point.x = 111;
        if (fail_grid_lookup == grid_lookups || address != FIRST)
            return 0;
        return &grid;
    }
    ++box_lookups;
    if (address == FIRST)
        return &first;
    return address == SECOND ? &second : 0;
}

_Noreturn void bfm_forest_model_invalid_handle(u32 address)
{
    rejected = address;
    longjmp(unavailable, 1);
}

s32 func_80135480(FloorContactRecord *object, u32 packed, FloorPoint *from, FloorPoint *to)
{
    assert(object == &record && from == &start && to == &end);
    (void)packed;
    return kind;
}

s32 func_80135EB0(FloorBox *box, s32 flags)
{
    assert(flags == -32768);
    ++box_calls;
    if (box == &first) {
        /* The next link can change while this child is running. */
        first.next = next_after_call;
        return 0;
    }
    assert(box == &second || box == &D_801D9524);
    return 1;
}

s32 func_80133AB0(s16 flags, s16 x, s16 z, FloorGrid *model)
{
    assert(model == &grid && flags == 0);
    assert(x == -9 && z == 17);
    assert(D_801D9500 == 0 && D_801D94FC == 0);
    if (!grid_calls) {
        assert(near_point.w == -32767 && far_point.w == 32767);
        near_point.w = -3019;
        far_point.x = 40;
        far_point.y = 41;
        far_point.z = 42;
    }
    return ++grid_calls <= retry_limit;
}

void func_80136A94(s32 rotate, FloorContactRecord *object, FloorPoint *result, void *matrix)
{
    assert(rotate == (kind == 2 || kind == 4));
    assert(object == &record && result == &end);
    assert(matrix == (kind == 4 ? (void *)&D_801D9504 : &record.rotation));
    ++transforms;
}

int main(void)
{
    kind = 1;
    assert(func_80135A4C(&record, (s32)FIRST, &start, &end) == 1);
    assert(box_calls == 2 && transforms == 1);

    kind = 0;
    assert(func_80135A4C(&record, (s32)FIRST, &start, &end) == 0);
    assert(box_calls == 2 && transforms == 1);

    kind = 2;
    start.y = 1;
    near_point.x = (u16)-9;
    near_point.z = 17;
    projected.w = 123;
    D_801152B0.w = -77;
    retry_limit = 2;
    assert(func_80135A4C(&record, FIRST & 0x0fffffffu, &start, &end) == 1);
    assert(grid_calls == 3 && grid_lookups == 3 && transforms == 2);
    assert(projected.x == 40 && projected.y == 41 && projected.z == 42 && projected.w == 123);
    assert(D_801152A8.w == -77);

    grid_calls = 0;
    retry_limit = 6;
    assert(func_80135A4C(&record, FIRST & 0x0fffffffu, &start, &end) == 0);
    assert(grid_calls == 6 && grid_lookups == 9 && transforms == 2);

    /* A missing required address transfers control to the owner. A zero link ends the list. */
    kind = 1;
    next_after_call = 0;
    assert(!func_80135A4C(&record, (s32)FIRST, &start, &end));
    assert(box_lookups == 3 && box_calls == 3 && transforms == 2);
    next_after_call = MISSING;
    if (setjmp(unavailable) == 0) {
        func_80135A4C(&record, (s32)FIRST, &start, &end);
        assert(0 && "unmapped nonzero successor");
    }
    assert(rejected == MISSING && box_calls == 4 && box_lookups == 5);
    if (setjmp(unavailable) == 0) {
        func_80135A4C(&record, (s32)MISSING, &start, &end);
        assert(0 && "unmapped first box");
    }
    assert(rejected == MISSING && box_calls == 4 && box_lookups == 6);

    /* Each retry observes the current map and the coordinates loaded for that call. */
    kind = 2;
    near_point.x = (u16)-9;
    retry_limit = 1;
    grid_calls = 0;
    change_x_on_lookup = 1;
    fail_grid_lookup = grid_lookups + 2;
    if (setjmp(unavailable) == 0) {
        func_80135A4C(&record, FIRST & 0x0fffffffu, &start, &end);
        assert(0 && "grid mapping withdrawn between attempts");
    }
    assert(rejected == FIRST && grid_lookups == 11 && grid_calls == 1);
    assert(near_point.x == 111 && near_point.w == -3019 && transforms == 2);

    /* Locally built boxes and culled segments do not use the tagged first handle. */
    kind = 3;
    assert(func_80135A4C(&record, (s32)MISSING, &start, &end));
    kind = 4;
    assert(func_80135A4C(&record, (s32)MISSING, &start, &end));
    kind = 0;
    assert(!func_80135A4C(&record, (s32)MISSING, &start, &end));
    assert(box_calls == 6 && box_lookups == 6 && transforms == 4);
    return 0;
}

/* The model supplies guest addresses. Only a zero successor ends a box chain. */
#include <assert.h>
#include <setjmp.h>
#include "forest_floor.h"

enum {
    RECORD = 0x00100010, GRID = 0x00202000, FIRST = 0x80101000u,
    NEXT = 0x80101010u, UNMAPPED = 0x80800000u
};
static jmp_buf unavailable;
static u32 rejected;
static FloorContactRecord model;
static FloorPoint probe = {40, (u16)-30, 22, -7};
static FloorGrid grid;
static FloorBox first, next;
FloorBox D_801D9524;
FloorMatrix D_801D9504;
static int classification, boxes, grids, transforms;
static FloorBox *accept;

void *bfm_forest_contact_pointer(u32 address, u32 width)
{
    assert(width == sizeof model);
    return address == RECORD ? &model : 0;
}

void *bfm_forest_contact_model(u32 address, s32 is_box)
{
    if (!is_box && address == GRID)
        return &grid;
    if (is_box && address == FIRST)
        return &first;
    if (is_box && address == NEXT)
        return &next;
    return 0;
}

_Noreturn void bfm_forest_model_invalid_handle(u32 address)
{
    rejected = address;
    longjmp(unavailable, 1);
}

s32 func_80135480(FloorContactRecord *record, u32 geometry, FloorPoint *start, FloorPoint *end)
{
    (void)geometry;
    assert(record == &model && start == &probe);
    assert(end->x == start->x && end->z == start->z && end->y == (u16)(start->y + 768));
    return classification;
}

s32 func_80135EB0(FloorBox *box, s32 flags)
{
    assert(flags == 0);
    ++boxes;
    return box == accept;
}

s32 func_801345F8(FloorGrid *map)
{
    assert(map == &grid);
    ++grids;
    return 1;
}

void func_80136A94(s32 rotate, FloorContactRecord *record, FloorPoint *result, void *matrix)
{
    assert(record == &model && result == &probe);
    assert(rotate == (classification == 2 || classification == 4));
    assert(matrix == (classification == 4 ? (void *)&D_801D9504 : &model.rotation));
    ++transforms;
}

static void expect_unavailable(u32 record, u32 geometry, u32 address)
{
    if (setjmp(unavailable) == 0) {
        func_80135D20(record, geometry, &probe);
        assert(0 && "missing guest mapping must return control to its owner");
    }
    assert(rejected == address);
}

int main(void)
{
    classification = 0;
    assert(func_80135D20(RECORD, FIRST, &probe) == 0);
    assert(!boxes && !grids && !transforms);

    classification = 2;
    first.next = NEXT;
    accept = &next;
    assert(func_80135D20(RECORD, FIRST, &probe) == 1);
    assert(boxes == 2 && !grids && transforms == 1);

    classification = 4;
    accept = &D_801D9524;
    assert(func_80135D20(RECORD, FIRST, &probe) == 1);
    assert(boxes == 3 && transforms == 2);

    classification = 1;
    assert(func_80135D20(RECORD, GRID, &probe) == 1 && grids == 1 && transforms == 3);
    first.next = 0;
    accept = 0;
    assert(func_80135D20(RECORD, FIRST, &probe) == 0 && boxes == 4 && transforms == 3);

    expect_unavailable(0, GRID, 0);
    expect_unavailable(RECORD + 4, GRID, RECORD + 4);
    expect_unavailable(RECORD, GRID + 4, GRID + 4);
    expect_unavailable(RECORD, UNMAPPED, UNMAPPED);
    first.next = UNMAPPED;
    expect_unavailable(RECORD, FIRST, UNMAPPED);
    assert(boxes == 5 && grids == 1 && transforms == 3);
    return 0;
}

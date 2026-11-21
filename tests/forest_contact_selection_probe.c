#define _GNU_SOURCE
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include "forest_floor.h"
typedef struct { s32 x, y, z, pad; } LongVector;
FloorMatrix D_801D9504;
FloorBox D_801D9524;
static FloorPoint near_point, far_point, movement;
FloorPoint *D_801870AC = &near_point, *D_801870B0 = &far_point, *D_801870B8 = &movement;
static FloorContactRecord record;
static FloorPoint start, end;
static FloorBox *first, *second, *expected_box[2];
static void *expected_model, *expected_matrix;
static unsigned list_calls, grid_calls, transform_calls, hits_on, matrix_calls;
static int rotates, overlapping;

/* Identity matrix services expose the selector's calls and in-place arguments. */
void func_80048EAC(void *matrix, void *result)
{
    assert(matrix == &record.rotation && result == &D_801D9504 && !matrix_calls);
    ++matrix_calls;
}
void func_8004D504(void *matrix, LongVector *in, LongVector *out)
{
    assert((matrix == &record.rotation || matrix == &D_801D9504) && in == out);
    ++matrix_calls;
}
void func_80048B6C(LongVector *in, LongVector *out)
{
    assert(in == out && matrix_calls > 0);
    ++matrix_calls;
}
void func_8004914C(void *matrix)
{
    assert(matrix);
    ++matrix_calls;
}
void func_8004864C(FloorPoint *in, s32 *out)
{
    out[0] = (s16)in->x;
    out[1] = (s16)in->y;
    out[2] = (s16)in->z;
    ++matrix_calls;
}
s32 func_80135EB0(FloorBox *box, s32 flags)
{
    assert(!flags && list_calls < 2 && box == expected_box[list_calls]);
    return ++list_calls == hits_on;
}
s32 func_80134A74(s32 flags, s16 x, s16 z, FloorGrid *model)
{
    assert(!flags && x == -13 && model == expected_model);
    assert(D_801870B8->x == 17);
    if (overlapping) {
        assert(z == -18 && D_801870B8->y == (u16)-18 && D_801870B8->z == 26);
    } else {
        assert(z == -17 && D_801870B8->y == (u16)-10 && D_801870B8->z == 25);
    }
    return ++grid_calls == hits_on;
}
void func_80136A94(s32 rotate, FloorContactRecord *actor, FloorPoint *output, void *matrix)
{
    assert(rotate == rotates && actor == &record && output == &end && matrix == expected_matrix);
    ++transform_calls;
    output->w = 11;
}

static void reset(void)
{
    start = (FloorPoint){(u16)-10, 13, (u16)-12, 1};
    end = (FloorPoint){7, 3, 13, 2};
    record.scale_x = record.scale_y = record.scale_z = 4096;
    record.x = 3;
    record.y = 4;
    record.z = 5;
    movement = (FloorPoint){80, 81, 82, 83};
    list_calls = grid_calls = transform_calls = matrix_calls = 0;
    expected_model = first;
    expected_matrix = &record.rotation;
    hits_on = 1;
    rotates = overlapping = 0;
    D_801870AC = &near_point;
    D_801870B8 = &movement;
}

static int query(u32 flags)
{
    u32 address = (u32)(uintptr_t)first & 0x0fffffffu;
    return func_80135888(&record, (s32)(flags | address), &start, &end);
}

int main(void)
{
    assert(offsetof(FloorContactRecord, scale_x) == 0x18 && offsetof(FloorContactRecord, rotation) == 0x34);
    assert(offsetof(FloorContactRecord, x) == 0x48 && sizeof(FloorMatrix) == 32 && sizeof(FloorBox) == 16);
    void *const address = (void *)(uintptr_t)0x801b4000;
    void *page = mmap(address, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page != address) {
        if (page != MAP_FAILED) munmap(page, 4096);
        return 77;
    }
    first = page;
    second = first + 1;
    first->next = (u32)(uintptr_t)second;
    first->xlo = -9; first->xhi = 13;
    first->ylo = -7; first->yhi = 5;
    first->zlo = 3; first->zhi = 27;

    reset();
    start.x = 900;
    assert(!query(0) && !list_calls && !grid_calls && !transform_calls && !matrix_calls);
    reset();
    assert(query(0) && grid_calls == 1 && !list_calls && transform_calls == 1 && end.w == 11);
    reset();
    rotates = 1;
    hits_on = 0;
    assert(!query(0x40000000u) && grid_calls == 1 && !transform_calls && matrix_calls == 2);

    reset();
    overlapping = 1;
    D_801870AC = (FloorPoint *)((u8 *)page + 0x200);
    D_801870B8 = (FloorPoint *)((u8 *)D_801870AC + sizeof(u16));
    assert(query(0) && grid_calls == 1 && transform_calls == 1);

    for (unsigned at = 0; at <= 2; ++at) {
        reset();
        hits_on = at;
        expected_box[0] = first;
        expected_box[1] = second;
        assert(query(0x80000000u) == (at != 0));
        assert(list_calls == (at == 1 ? 1u : 2u) && transform_calls == (at != 0));
        assert(!grid_calls && !matrix_calls && movement.x == 80);
    }
    for (unsigned mode = 3; mode <= 4; ++mode) {
        reset();
        expected_box[0] = &D_801D9524;
        rotates = mode == 4;
        if (rotates) expected_matrix = &D_801D9504;
        assert(query(rotates ? 0xe0000000u : 0xa0000000u));
        assert(list_calls == 1 && !grid_calls && transform_calls == 1);
        assert(matrix_calls == (rotates ? 6u : 3u));
        assert(!D_801D9524.next && D_801D9524.xlo == -9 && D_801D9524.zhi == 27);
    }
    munmap(page, 4096);
}

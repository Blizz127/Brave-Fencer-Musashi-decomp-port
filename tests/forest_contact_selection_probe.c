#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "forest_floor.h"

#define FIRST_HANDLE UINT32_C(0x801b4000)
#define SECOND_HANDLE UINT32_C(0x801b4100)
#define UNCACHED_SECOND_HANDLE UINT32_C(0xa01b4100)
#define UNKNOWN_HANDLE UINT32_C(0x801b4300)

typedef struct { s32 x, y, z, pad; } LongVector;
FloorMatrix D_801D9504;
FloorBox D_801D9524;
static FloorPoint near_point, far_point, movement;
FloorPoint *D_801870AC = &near_point, *D_801870B0 = &far_point, *D_801870B8 = &movement;
static FloorContactRecord record;
static FloorPoint start, end;
static FloorBox *first, *second, *expected_box[2];
static FloorGrid *grid;
static void *expected_matrix;
static unsigned list_calls, hits_on;
static int rotates, overlapping, change_next, change_near, change_bounds;
static u32 replacement_next, refused, last_resolved;
static jmp_buf escape;
static int catching;
static char events[32];
static size_t event_count;

/* b/g are typed model resolutions, B/G are contact tests, T applies the hit. */
static void event(char kind)
{
    assert(event_count + 1 < sizeof events);
    events[event_count++] = kind;
    events[event_count] = '\0';
}

void *bfm_forest_contact_model(u32 address, s32 is_box)
{
    assert(address && (is_box == 0 || is_box == 1));
    last_resolved = address;
    event(is_box ? 'b' : 'g');
    if (is_box) {
        if (address == FIRST_HANDLE) return first;
        if (address == SECOND_HANDLE || address == UNCACHED_SECOND_HANDLE) return second;
    } else {
        /* All three movement coordinates, and the child call's signed x/z
         * arguments, are evaluated before the child needs its host grid. */
        assert(D_801870B8->x == 17);
        assert(D_801870B8->y == (overlapping ? (u16)-18 : (u16)-10));
        assert(D_801870B8->z == (overlapping ? 26u : 25u));
        if (change_near) {
            D_801870AC->x = 43;
            D_801870AC->z = 51;
        }
        if (address == FIRST_HANDLE) return grid;
    }
    return NULL;
}

_Noreturn void bfm_forest_model_invalid_handle(u32 address)
{
    assert(catching);
    event('!');
    refused = address;
    longjmp(escape, 1);
}

/* The identity services expose the selector's calls and in-place arguments. */
void func_80048EAC(void *matrix, void *result)
{
    assert(matrix == &record.rotation && result == &D_801D9504);
    event('I');
}
void func_8004D504(void *matrix, LongVector *in, LongVector *out)
{
    assert((matrix == &record.rotation || matrix == &D_801D9504) && in == out);
    event('L');
}
void func_80048B6C(LongVector *in, LongVector *out)
{
    assert(in == out);
    event('E');
}
void func_8004914C(void *matrix)
{
    assert(matrix);
    if (change_bounds) first->xlo = -123;
    event('S');
}
void func_8004864C(FloorPoint *in, s32 *out)
{
    out[0] = (s16)in->x;
    out[1] = (s16)in->y;
    out[2] = (s16)in->z;
    event('V');
}
s32 func_80135EB0(FloorBox *box, s32 flags)
{
    assert(!flags && list_calls < 2 && box == expected_box[list_calls]);
    event('B');
    if (list_calls == 0 && change_next) box->next = replacement_next;
    return ++list_calls == hits_on;
}
s32 func_80134A74(s32 flags, s16 x, s16 z, FloorGrid *model)
{
    assert(!flags && x == -13 && model == grid);
    assert(D_801870B8->x == 17);
    if (overlapping) {
        assert(z == -18 && D_801870B8->y == (u16)-18 && D_801870B8->z == 26);
    } else {
        assert(z == -17 && D_801870B8->y == (u16)-10 && D_801870B8->z == 25);
    }
    event('G');
    return hits_on != 0;
}
void func_80136A94(s32 rotate, FloorContactRecord *actor, FloorPoint *output, void *matrix)
{
    assert(rotate == rotates && actor == &record && output == &end && matrix == expected_matrix);
    event('T');
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
    near_point = (FloorPoint){61, 62, 63, 64};
    far_point = (FloorPoint){71, 72, 73, 74};
    movement = (FloorPoint){80, 81, 82, 83};
    *first = (FloorBox){SECOND_HANDLE, -9, 13, -7, 5, 3, 27};
    *second = (FloorBox){0, 0, 0, 0, 0, 0, 0};
    D_801D9524 = (FloorBox){UNKNOWN_HANDLE, -31, 32, -33, 34, -35, 36};
    list_calls = 0;
    expected_box[0] = first;
    expected_box[1] = second;
    expected_matrix = &record.rotation;
    hits_on = 1;
    rotates = overlapping = change_next = change_near = change_bounds = 0;
    last_resolved = 0;
    event_count = 0;
    events[0] = '\0';
    D_801870AC = &near_point;
    D_801870B8 = &movement;
}

static int query(u32 flags, u32 handle)
{
    return func_80135888(&record, (s32)(flags | (handle & 0x0fffffffu)), &start, &end);
}

static void check_query(u32 flags, u32 handle, int expected, const char *order)
{
    assert(query(flags, handle) == expected);
    assert(strcmp(events, order) == 0);
    assert(end.w == (expected ? 11 : 2));
}

static void check_refusal(u32 flags, u32 handle, u32 expected, const char *order)
{
    refused = 0;
    catching = 1;
    if (setjmp(escape) == 0) {
        (void)query(flags, handle);
        abort();
    }
    catching = 0;
    assert(refused == expected && strcmp(events, order) == 0);
    assert(end.w == 2);
}

int main(void)
{
    FloorBox boxes[2];
    FloorGrid grid_storage = {0};
    /* Allocated storage permits the halfword-shifted overlap of the two views. */
    void *overlap = calloc(1, sizeof(FloorPoint) + sizeof(u16));
    assert(overlap);
    first = &boxes[0];
    second = &boxes[1];
    grid = &grid_storage;
    assert(offsetof(FloorContactRecord, scale_x) == 0x18 && offsetof(FloorContactRecord, rotation) == 0x34);
    assert(offsetof(FloorContactRecord, x) == 0x48 && sizeof(FloorMatrix) == 32 && sizeof(FloorBox) == 16);

    reset();
    start.x = 900;
    check_query(0, UNKNOWN_HANDLE, 0, "");
    reset();
    start.x = 900;
    check_query(0xe0000000u, UNKNOWN_HANDLE, 0, "");
    reset();
    change_near = 1;
    check_query(0, FIRST_HANDLE, 1, "gGT");
    assert(near_point.x == 43 && near_point.z == 51);
    reset();
    rotates = 1;
    hits_on = 0;
    check_query(0x40000000u, FIRST_HANDLE, 0, "LEgG");
    reset();
    check_query(0x20000000u, FIRST_HANDLE, 1, "gGT");
    reset();
    rotates = 1;
    check_query(0x60000000u, FIRST_HANDLE, 1, "LEgGT");
    reset();
    overlapping = 1;
    D_801870AC = overlap;
    D_801870B8 = (FloorPoint *)((u8 *)overlap + sizeof(u16));
    check_query(0, FIRST_HANDLE, 1, "gGT");

    for (unsigned at = 0; at <= 2; ++at) {
        reset();
        hits_on = at;
        check_query(0x80000000u, FIRST_HANDLE, at != 0,
                    at == 0 ? "bBbB" : (at == 1 ? "bBT" : "bBbBT"));
        assert(movement.x == 80);
    }
    reset();
    rotates = 1;
    check_query(0xc0000000u, FIRST_HANDLE, 1, "LEbBT");
    reset();
    first->next = UNKNOWN_HANDLE;
    change_next = 1;
    replacement_next = SECOND_HANDLE;
    hits_on = 2;
    check_query(0x80000000u, FIRST_HANDLE, 1, "bBbBT");
    reset();
    first->next = UNCACHED_SECOND_HANDLE;
    hits_on = 2;
    check_query(0x80000000u, FIRST_HANDLE, 1, "bBbBT");
    assert(last_resolved == UNCACHED_SECOND_HANDLE);
    reset();
    first->next = 0;
    hits_on = 0;
    check_query(0x80000000u, FIRST_HANDLE, 0, "bB");
    reset();
    first->next = UNKNOWN_HANDLE;
    check_query(0x80000000u, FIRST_HANDLE, 1, "bBT");

    for (unsigned mode = 3; mode <= 4; ++mode) {
        reset();
        expected_box[0] = &D_801D9524;
        rotates = mode == 4;
        if (rotates) expected_matrix = &D_801D9504;
        change_bounds = 1;
        check_query(rotates ? 0xe0000000u : 0xa0000000u, FIRST_HANDLE, 1,
                    rotates ? "ILESbVVBT" : "SbVVBT");
        assert(!D_801D9524.next && D_801D9524.xlo == -123 && D_801D9524.zhi == 27);
    }
    reset();
    expected_box[0] = &D_801D9524;
    hits_on = 0;
    check_query(0xa0000000u, FIRST_HANDLE, 0, "SbVVB");

    reset();
    check_refusal(0, SECOND_HANDLE, SECOND_HANDLE, "g!");
    assert(movement.x == 17 && movement.y == (u16)-10 && movement.z == 25);
    reset();
    check_refusal(0x80000000u, 0, 0x80000000u, "b!");
    reset();
    check_refusal(0xc0000000u, UNKNOWN_HANDLE, UNKNOWN_HANDLE, "LEb!");
    reset();
    change_next = 1;
    replacement_next = UNKNOWN_HANDLE;
    hits_on = 0;
    check_refusal(0x80000000u, FIRST_HANDLE, UNKNOWN_HANDLE, "bBb!");
    reset();
    check_refusal(0xa0000000u, UNKNOWN_HANDLE, UNKNOWN_HANDLE, "Sb!");
    assert(near_point.x == 61 && far_point.x == 71 && D_801D9524.next == UNKNOWN_HANDLE);
    reset();
    check_refusal(0xe0000000u, UNKNOWN_HANDLE, UNKNOWN_HANDLE, "ILESb!");
    assert(near_point.x == 61 && far_point.x == 71 && D_801D9524.xlo == -31);
    free(overlap);
    return 0;
}

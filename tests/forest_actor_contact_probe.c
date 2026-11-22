#include "forest_actor_contact.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(ForestContactSlot) == 0x10c, "actor slot stride");
_Static_assert(offsetof(ForestContactSlot, field06) == 6, "first contact field");
_Static_assert(offsetof(ForestContactSlot, field0a) == 10, "additional contact field");
_Static_assert(offsetof(ForestContactSlot, field0e) == 14, "last contact field");
_Static_assert(offsetof(ForestContactSlot, record) == 0x20, "actor record handle");
_Static_assert(offsetof(ForestContactSlot, collision) == 0x58, "packed collision model");
_Static_assert(offsetof(ForestContactSlot, flags) == 0x5c, "actor flags");

enum { SLOT_COUNT = 96, OBJECT_SIZE = 0x18c, RECORD_BASE = 0x80040000u,
       SLOT_BASE = 0x801202a0u, START = 0x1f800060u, END = 0x1f800068u };
ForestContactSlot D_801202A0[SLOT_COUNT];
static FloorContactRecord records[SLOT_COUNT];
static FloorPoint start = {12, (u16)-80, 35, 1}, end = {20, 36, 47, 2};
static unsigned visits[SLOT_COUNT], visit_count;
typedef s32 (*Search)(u8 *, u32, u32);

void *bfm_forest_contact_pointer(u32 address, u32 width)
{
    if (address == START || address == END) {
        assert(width == sizeof(FloorPoint));
        return address == START ? &start : &end;
    }
    assert(width == sizeof(FloorContactRecord) && address >= RECORD_BASE);
    u32 offset = address - RECORD_BASE;
    assert(offset % 0x80 == 0 && offset / 0x80 < SLOT_COUNT);
    return &records[offset / 0x80];
}

static u32 collision_word(unsigned index)
{
    return 0x90008000u + index * 0x20u;
}

/* Each eligible record supplies a horizontal test plane for this segment. */
s32 func_80135888(FloorContactRecord *record, s32 packed, FloorPoint *from, FloorPoint *to)
{
    assert(from == &start && to == &end);
    unsigned index = (unsigned)(record - records);
    assert(index < SLOT_COUNT && (u32)packed == collision_word(index));
    assert(visit_count < SLOT_COUNT);
    visits[visit_count++] = index;
    return (s16)from->y < (s32)record->y && (s32)record->y <= (s16)to->y;
}

static void actor(unsigned i, u16 flags, s32 height)
{
    ForestContactSlot *slot = &D_801202A0[i];
    slot->id = (u16)(0x100u + i);
    slot->flags = flags;
    slot->collision = collision_word(i);
    slot->record = RECORD_BASE + i * 0x80u;
    slot->field06 = (u16)(0x1340u + i);
    slot->field0a = (u16)(0x2850u + i);
    slot->field0e = (u16)(0x3c60u + i);
    records[i].y = (u32)height;
}

static void check_visits(const unsigned *expected, unsigned count)
{
    assert(visit_count == count);
    for (unsigned i = 0; i < count; ++i)
        assert(visits[i] == expected[i]);
}

static void expect_half(u8 *object, unsigned offset, u16 value)
{
    memcpy(object + offset, &value, sizeof value);
}

static void check_search(Search search, u16 mask, int winner, const unsigned *order, unsigned visits_needed)
{
    _Alignas(8) u8 object[OBJECT_SIZE], expected[OBJECT_SIZE];
    memset(object, 0xad, sizeof object);
    memcpy(expected, object, sizeof object);
    if (winner >= 0) {
        u32 handle = SLOT_BASE + (u32)winner * 0x10cu;
        memcpy(expected + (mask == 0x40 ? 0x17c : 0x180), &handle, 4);
        expect_half(expected, 6, (u16)(0x1340 + winner));
        if (mask == 0x20)
            expect_half(expected, 10, (u16)(0x2850 + winner));
        expect_half(expected, 14, (u16)(0x3c60 + winner));
    }
    visit_count = 0;
    assert(search(object, START, END) == (winner >= 0));
    check_visits(order, visits_needed);
    /* Includes the byte immediately after each handle and each halfword. */
    assert(memcmp(object, expected, sizeof object) == 0);
}

static void exercise_filters(Search search, u16 mask)
{
    const unsigned initial[] = {0}, first[] = {7, 26}, last[] = {7, 95}, misses[] = {7, 26, 95};
    memset(D_801202A0, 0, sizeof D_801202A0);
    check_search(search, mask, -1, NULL, 0);
    actor(0, mask, -12);
    check_search(search, mask, 0, initial, 1);
    D_801202A0[0].id = 0;
    actor(2, mask ^ 0x60u, -16);
    actor(3, mask, -20);
    D_801202A0[3].collision = 0;
    actor(7, mask, 150);
    actor(26, mask | 0x80u, -32);
    actor(95, 0x60, 12);
    check_search(search, mask, 26, first, 2);
    D_801202A0[26].id = 0;
    check_search(search, mask, 95, last, 2);
    actor(26, mask, -100);
    actor(95, mask, 64);
    check_search(search, mask, -1, misses, 3);
}

/* Lay the result handle over the winning actor's field06 or field0a. Its
 * upper half must be visible when that field is copied after the handle. */
static void exercise_alias(Search search, u16 mask)
{
    const unsigned winner = 42, order[] = {winner};
    unsigned handle_offset = mask == 0x40 ? 0x17c : 0x180;
    unsigned overlap = mask == 0x40 ? 4 : 8;
    u8 *table = (u8 *)&D_801202A0;
    u8 *object = table + winner * sizeof(ForestContactSlot) + overlap - handle_offset;
    u8 expected[OBJECT_SIZE];
    memset(D_801202A0, 0, sizeof D_801202A0);
    actor(winner, mask, 0);
    /* Canaries immediately following the four-byte destination also lie in the slot. */
    memset(object + handle_offset + 4, 0xbf, 2);
    memcpy(expected, object, sizeof expected);
    u32 handle = SLOT_BASE + winner * 0x10cu;
    memcpy(expected + handle_offset, &handle, 4);
    expect_half(expected, 6, mask == 0x40 ? (u16)(handle >> 16) : D_801202A0[winner].field06);
    if (mask == 0x20)
        expect_half(expected, 10, (u16)(handle >> 16));
    expect_half(expected, 14, D_801202A0[winner].field0e);
    visit_count = 0;
    assert(search(object, START, END) == 1);
    check_visits(order, 1);
    assert(memcmp(object, expected, sizeof expected) == 0);
}

int main(void)
{
    exercise_filters(func_8014E5B4, 0x40);
    exercise_filters(func_8014E83C, 0x20);
    exercise_alias(func_8014E5B4, 0x40);
    exercise_alias(func_8014E83C, 0x20);
    return 0;
}

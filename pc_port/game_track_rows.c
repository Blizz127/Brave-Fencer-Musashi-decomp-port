#include "game_track_rows.h"
#include <stdint.h>

_Static_assert(sizeof(TrackDefinition) == 12, "track definition stride");
_Static_assert(sizeof(TrackPair) == 2, "track pair stride");
_Static_assert(sizeof(TrackRow) == 12, "track row stride");
_Static_assert(sizeof(TrackInfo) == 32, "audio service result size");
_Static_assert(sizeof(TrackSlot) == 24, "slot stride");
_Static_assert(sizeof(D_800C5328[0]) == 4, "owner stride");
_Static_assert(offsetof(TrackControl, track) == 0x98, "track selection offset");

static _Thread_local TrackRowWindows current;
static _Thread_local int data_fault;
int bfm_track_rows_failed(void)
{
    return data_fault;
}

s32 bfm_reject_track_rows(void)
{
    data_fault = 1;
    return 0;
}

void bfm_bind_track_rows(const TrackRowWindows *windows)
{
    data_fault = 0;
    if (windows)
        current = *windows;
    else
        current = (TrackRowWindows){0};
}

int bfm_track_slot_valid(s16 slot)
{
    size_t count = sizeof(D_800A4638.slots) / sizeof(D_800A4638.slots[0]);
    return slot >= 0 && (size_t)slot < count && (size_t)slot < current.slot_table_count;
}

int bfm_track_definition_valid(s16 track)
{
    return track >= 0 && (size_t)track < current.definition_count;
}

int bfm_track_owners_valid(s16 track, u8 kind, u8 target)
{
    return track >= 0 && (size_t)track < current.owner_count &&
           (!kind || (size_t)target < current.owner_count);
}

/* Validate a descriptor before forming any host pointer. The source address
 * may designate a subarray inside a bound window. Offset and span arithmetic
 * must fit both the bound host array and the 32-bit source address space. */
static int window_index(u32 address, u32 base, size_t capacity, size_t stride,
                        size_t alignment, size_t first, size_t count, size_t *index)
{
    const uint64_t end = UINT64_C(1) << 32;
    if (address < base || base % alignment || address % alignment ||
        (address - base) % stride || capacity > (end - base) / stride)
        return 0;
    size_t start = (address - base) / stride;
    if (start > capacity || first > capacity - start ||
        count > capacity - start - first)
        return 0;
    *index = start + first;
    return 1;
}

int bfm_track_chunk(u32 pair_address, u32 row_address, s32 first, s32 remaining,
                    const TrackPair **pairs, TrackRow **rows)
{
    size_t pair_index, row_index;
    *pairs = NULL;
    *rows = NULL;
    if (remaining <= 0)
        return 1;
    size_t count = (size_t)(remaining < 10 ? remaining : 10);
    if (first < 0 || !current.pairs || !current.rows ||
        !window_index(pair_address, current.pair_base, current.pair_count,
                      sizeof(TrackPair), _Alignof(TrackPair), (size_t)first, count, &pair_index) ||
        !window_index(row_address, current.row_base, current.row_count,
                      sizeof(TrackRow), _Alignof(TrackRow), (size_t)first, count, &row_index))
        return 0;
    *pairs = current.pairs + pair_index;
    *rows = current.rows + row_index;
    return 1;
}

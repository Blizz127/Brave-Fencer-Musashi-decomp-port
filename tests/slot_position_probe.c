#include "slot_position_updates.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

enum {
    SLOT_COUNT = 20,
    TABLE = 0x9c28, REPLACEMENT_TABLE = 0xab64,
    CHANGE = 0x1350, LATE = 0x18bc, UNMAPPED = 0x28b0,
    LARGE = 0x50a4, SMALL = 0x527c, LINKED = 0x53e8, CHILD = 0x6350
};

ScenePositionSlot D_8011F9D0[SLOT_COUNT];
u32 D_80126D40;
static ScenePositionTarget large_target, small_target, linked_target;
static ScenePositionAttachment child;
static u32 entries[3];
static unsigned table_reads, default_calls, changes, late_calls, refusals;
static enum ScenePositionRefusal last_refusal;
static u32 refused_key;
static unsigned refused_detail;

static u32 load_le(const void *field, size_t width)
{
    const u8 *bytes = field;
    u32 value = 0;
    for (size_t i = 0; i < width; ++i)
        value |= (u32)bytes[i] << (8 * i);
    return value;
}

static void store_le(void *field, size_t width, u32 value)
{
    u8 *bytes = field;
    for (size_t i = 0; i < width; ++i)
        bytes[i] = (u8)(value >> (8 * i));
}

#define SHORT(field) load_le(&(field), sizeof(u16))
#define WORD(field) load_le(&(field), sizeof(u32))
#define SET(field, value) store_le(&(field), sizeof(field), (u32)(value))

static void change(ScenePositionSlot *slot)
{
    assert(slot == &D_8011F9D0[1]);
    assert(slot->prevX == slot->x && slot->prevY == slot->y && slot->prevZ == slot->z);
    assert(!late_calls && !default_calls);
    ++changes;
    slot->x = INT16_MIN;
    slot->y = -57;
    slot->z = INT16_MAX;
    slot->kind = 0;
    slot->target = LARGE;

    /* The next live slot still reads this table, including its edited entry. */
    D_80126D40 = REPLACEMENT_TABLE;
    entries[1] = LATE;
    ScenePositionSlot *last = &D_8011F9D0[SLOT_COUNT - 1];
    assert(last->kind == 0 && last->prevY == 0);
    last->kind = 2;
    last->y = 2237;
}

static void late(ScenePositionSlot *slot)
{
    assert(changes == 1);
    ++late_calls;
    if (slot == &D_8011F9D0[3]) {
        assert(late_calls == 1 && default_calls == 0);
        assert(WORD(large_target.x48) == (u32)INT16_MIN && WORD(large_target.y4C) == (u32)-57);
        slot->target = LINKED;
        slot->x = -975;
        slot->y = INT16_MAX;
        slot->z = INT16_MIN;
    } else {
        assert(slot == &D_8011F9D0[SLOT_COUNT - 1] && late_calls == 2);
        assert(default_calls == 2 && slot->prevY == 2237);
        assert(SHORT(small_target.x) == 171 && WORD(child.y) == INT16_MAX);
    }
}

void func_80162CC4(ScenePositionSlot *slot)
{
    assert(late_calls == 1);
    ++default_calls;
    assert(slot == &D_8011F9D0[5] || slot == &D_8011F9D0[7]);
}

static void reset(void)
{
    memset(D_8011F9D0, 0, sizeof D_8011F9D0);
    memset(&large_target, 0, sizeof large_target);
    memset(&small_target, 0, sizeof small_target);
    memset(&linked_target, 0, sizeof linked_target);
    memset(&child, 0, sizeof child);
    D_80126D40 = TABLE;
    table_reads = changes = default_calls = late_calls = refusals = 0;
    entries[0] = CHANGE;
    entries[1] = UNMAPPED;
    entries[2] = 0;
    for (int i = 0; i < SLOT_COUNT; ++i) {
        D_8011F9D0[i].x = (s16)(32 * i + 11);
        D_8011F9D0[i].y = (s16)(-9 * i - 17);
        D_8011F9D0[i].z = (s16)(103 - i);
    }
}

static void populated_pool(void)
{
    reset();
    SET(large_target.kind, 1);
    SET(large_target.flags2C, 0x9e28);
    SET(linked_target.kind, 4);
    SET(linked_target.sub34, CHILD);
    SET(linked_target.flags2C, 0x3a22);
    SET(small_target.x48, -321779);
    D_8011F9D0[1].kind = 1;
    D_8011F9D0[3].kind = 2;
    D_8011F9D0[5].kind = 3;
    D_8011F9D0[7].kind = 3;
    D_8011F9D0[5].target = SMALL;

    func_80162B1C();

    assert(refusals == 0 && table_reads == 5 && changes == 1 && late_calls == 2 && default_calls == 2);
    for (int i = 0; i < SLOT_COUNT; ++i) {
        assert(D_8011F9D0[i].prevX == 32 * i + 11);
        assert(D_8011F9D0[i].prevY == (i == SLOT_COUNT - 1 ? 2237 : -9 * i - 17));
        assert(D_8011F9D0[i].prevZ == 103 - i);
    }
    assert(SHORT(large_target.x) == (u16)INT16_MIN && WORD(large_target.x48) == (u32)INT16_MIN);
    assert(SHORT(large_target.y) == (u16)-57 && WORD(large_target.y4C) == (u32)-57);
    assert(SHORT(large_target.z) == INT16_MAX && WORD(large_target.z50) == INT16_MAX);
    assert(SHORT(large_target.flags2C) == 0x9e29);
    assert(SHORT(linked_target.x) == (u16)-975 && SHORT(linked_target.y) == INT16_MAX);
    assert(SHORT(linked_target.z) == (u16)INT16_MIN);
    assert(WORD(child.x) == (u32)-975 && WORD(child.y) == INT16_MAX && WORD(child.z) == (u32)INT16_MIN);
    assert(SHORT(linked_target.flags2C) == 0x3a22 && WORD(linked_target.x48) == 0);
    assert(SHORT(small_target.x) == 171 && SHORT(small_target.y) == (u16)-62);
    assert(SHORT(small_target.z) == 98 && WORD(small_target.x48) == (u32)-321779);
    assert(SHORT(small_target.flags2C) == 0);
}

int bfm_position_handler_key(u32 table, u32 index, u32 *handler)
{
    ++table_reads;
    if (table != TABLE || index >= sizeof(entries) / sizeof(entries[0]))
        return 0;
    *handler = entries[index];
    return 1;
}

ScenePositionHandler bfm_position_handler(u32 key)
{
    if (key == CHANGE)
        return change;
    if (key == LATE)
        return late;
    return NULL;
}

u8 *bfm_position_target_bytes(u32 key, unsigned offset, unsigned width)
{
    if (offset >= sizeof(ScenePositionTarget) || width > sizeof(ScenePositionTarget) - offset)
        return NULL;
    switch (key) {
    case LARGE: return (u8 *)&large_target + offset;
    case SMALL: return (u8 *)&small_target + offset;
    case LINKED: return (u8 *)&linked_target + offset;
    default: return NULL;
    }
}

u8 *bfm_position_attachment_bytes(u32 key, unsigned offset, unsigned width)
{
    if (key != CHILD || offset >= sizeof(child) || width > sizeof(child) - offset)
        return NULL;
    return (u8 *)&child + offset;
}

void bfm_position_refused(enum ScenePositionRefusal reason, u32 key, unsigned detail)
{
    ++refusals;
    last_refusal = reason;
    refused_key = key;
    refused_detail = detail;
}

static void rejected_dispatch(int missing_entry)
{
    reset();
    ScenePositionSlot *active = &D_8011F9D0[4];
    active->kind = missing_entry ? 23 : 2;
    active->target = LARGE;
    func_80162B1C();
    assert(refusals == 1 && table_reads == 1 && default_calls == 0 && changes == 0 && late_calls == 0);
    assert(active->prevX == active->x && SHORT(large_target.x) == 0);
    assert(last_refusal == (missing_entry ? BFM_POSITION_BAD_TABLE : BFM_POSITION_BAD_HANDLER));
    assert(refused_key == (missing_entry ? TABLE : UNMAPPED));
    assert(refused_detail == (unsigned)active->kind - 1);
}

int main(void)
{
    if ((uintptr_t)D_8011F9D0 <= UINT32_MAX || (uintptr_t)&large_target <= UINT32_MAX ||
        (uintptr_t)&child <= UINT32_MAX)
        return 77;
    populated_pool();
    rejected_dispatch(1);
    rejected_dispatch(0);
    return 0;
}

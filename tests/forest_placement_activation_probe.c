#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../src/overlays/sc02_0035/8012c724.c"
#include "../src/overlays/main_0012/8013e298.c"
#include "../src/overlays/main_0012/8013e410.c"
#include "../src/overlays/main_0012/8013e448.c"

/* The walk uses pointer formals for these calls. Give it matching host
 * declarations and narrow only the slot word passed to the mark routine. */
#define func_8012C724 mark_chosen_slot
#define func_8013E410 find_free_slot
#include "../src/overlays/main_0012/8013e370.c"
#undef func_8012C724
#undef func_8013E410

enum { SLOT_COUNT = 96, ENTRY_COUNT = 7 };
_Static_assert(sizeof(Slot8013E410) == 268, "slot stride");
_Static_assert(sizeof(Ent8013E370) == 20, "placement stride");
_Static_assert(offsetof(Obj8012C724, flags) == offsetof(Ent8013E370, flags),
               "mark and walk must share the flags halfword");

Slot8013E410 D_801202A0[SLOT_COUNT + 1];
s16 D_80126CB0, D_80126CAC[3];
u32 D_801274CC, D_801274D0;
s32 D_801274D8, D_801274DC;

static Ent8013E370 entries[ENTRY_COUNT];
static unsigned int searches, predicates, distances, created;
static s32 distance_value;
static int mutate_during_walk;
static unsigned int entry_ids[ENTRY_COUNT], slot_ids[ENTRY_COUNT];
static s16 *last_position;
static Ent8013E370 *mark_entry;
static Obj8012C724 *mark_header;

static u32 low_address(uintptr_t address)
{
    assert(address <= INT32_MAX);
    return (u32)address;
}

void *find_free_slot(void)
{
    ++searches;
    return func_8013E410();
}

void mark_chosen_slot(Ent8013E370 *entry, void *slot)
{
    /* The two sources name the same header using different struct types.
     * Stage it in the callee's own type and copy its changes at callback entry. */
    Obj8012C724 header;
    assert(mark_entry == NULL && mark_header == NULL);
    memcpy(&header, entry, sizeof(header));
    mark_entry = entry;
    mark_header = &header;
    func_8012C724(&header, (s32)low_address((uintptr_t)slot));
    mark_header = NULL;
    mark_entry = NULL;
}

void func_8012C890(void *record, s32 slot_word, s32 owner)
{
    Ent8013E370 *entry = mark_entry;
    assert(mark_header != NULL && record == mark_header);
    assert(mark_header->flags == (entry->flags | 0x8000));
    assert(memcmp(entry, mark_header, offsetof(Obj8012C724, flags)) == 0);
    memcpy(entry, mark_header, sizeof(*mark_header));
    Slot8013E410 *slot = (Slot8013E410 *)(uintptr_t)(u32)slot_word;
    assert(owner == 0 && created < ENTRY_COUNT);
    assert(entry >= entries && entry < entries + ENTRY_COUNT);
    assert(slot >= D_801202A0 && slot < D_801202A0 + SLOT_COUNT);
    entry_ids[created] = (unsigned int)(entry - entries);
    slot_ids[created] = (unsigned int)(slot - D_801202A0);
    ++created;
    slot->used = entry->kind;
    if (mutate_during_walk && entry == entries + 2)
        entries[3].flags |= 0x8000;
}

s32 func_800132BC(s16 *center, s16 *position)
{
    assert(center == D_80126CAC);
    ++distances;
    last_position = position;
    return distance_value;
}

static s32 test_predicate(s16 *position)
{
    Obj8013E298 point = { .f4 = position[2] };
    ++predicates;
    last_position = position;
    if (mutate_during_walk && position == entries[2].pos) {
        D_801202A0[4].used = 23;
        D_801202A0[3].used = 0;
    }
    return func_8013E298(&point);
}

static s32 signed_hook_result(s16 *position)
{
    last_position = position;
    return -17;
}

static void reset_fixture(void)
{
    memset(entries, 0, sizeof(entries));
    memset(D_801202A0, 0, sizeof(D_801202A0));
    D_801274CC = low_address((uintptr_t)entries);
    D_801274D0 = low_address((uintptr_t)&test_predicate);
    D_80126CB0 = 100;
    searches = predicates = distances = created = 0;
    mutate_during_walk = 0;
    last_position = NULL;
}

static void test_distance_band(void)
{
    s16 position[3] = {3, -2, 1};
    reset_fixture();
    for (s32 sign = -1; sign <= 1; sign += 2) {
        const int gaps[] = {0, 704, 705, 767, 768, 900};
        for (unsigned int i = 0; i < sizeof(gaps) / sizeof(gaps[0]); ++i) {
            position[2] = (s16)(D_80126CB0 + sign * gaps[i]);
            assert(func_8013E448(position) == (gaps[i] >= 705 && gaps[i] <= 767));
            assert(last_position == position);
        }
    }
    D_80126CB0 = INT16_MIN;
    position[2] = INT16_MAX;
    assert(!func_8013E448(position));
    D_80126CB0 = INT16_MAX;
    position[2] = INT16_MIN;
    assert(!func_8013E448(position));
    assert(predicates == 14 && distances == 0);

    D_801274D0 = low_address((uintptr_t)&signed_hook_result);
    assert(func_8013E448(position) == -17 && distances == 0);
    D_801274D0 = 0;
    D_801274D8 = 8;
    D_801274DC = 12;
    for (distance_value = 7; distance_value <= 13; ++distance_value) {
        assert(func_8013E448(position) == (distance_value > 8 && distance_value < 12));
        assert(last_position == position);
    }
    D_801274D8 = INT32_MIN;
    D_801274DC = -2;
    distance_value = -3;
    assert(func_8013E448(position) == 1);
    distance_value = INT32_MIN;
    assert(func_8013E448(position) == 0);
    D_801274D8 = 7;
    D_801274DC = 5;
    distance_value = 6;
    assert(func_8013E448(position) == 0);
    assert(distances == 10);
}

static void test_first_free_slot(void)
{
    reset_fixture();
    for (unsigned int i = 0; i < SLOT_COUNT; ++i) {
        assert(func_8013E410() == &D_801202A0[i]);
        D_801202A0[i].used = 0x0100;
    }
    assert(D_801202A0[SLOT_COUNT].used == 0 && func_8013E410() == NULL);
}

static void test_empty_and_full(void)
{
    reset_fixture();
    D_801274CC = 0;
    func_8013E370();
    D_801274CC = low_address((uintptr_t)entries);
    func_8013E370();
    assert(searches == 0 && predicates == 0 && created == 0);

    for (unsigned int i = 0; i < SLOT_COUNT; ++i)
        D_801202A0[i].used = 1;
    entries[0].kind = 3;
    entries[0].flags = 0x8011;
    entries[1].kind = 4;
    entries[2].kind = 5;
    func_8013E370();
    assert(searches == 1 && predicates == 0 && created == 0);
    assert(entries[0].flags == 0x8011 && entries[1].flags == 0 && entries[2].flags == 0);
}

static void test_binding_order(void)
{
    reset_fixture();
    for (unsigned int i = 0; i < SLOT_COUNT; ++i)
        D_801202A0[i].used = 7;
    D_801202A0[4].used = 0;
    D_801202A0[SLOT_COUNT - 1].used = 0;
    for (unsigned int i = 0; i < ENTRY_COUNT; ++i) {
        entries[i].kind = (u16)(i + 1);
        entries[i].flags = (u16)(i + 10);
        entries[i].pos[2] = 820;
    }
    entries[0].flags |= 0x8000;
    entries[1].pos[2] = 0;
    entries[5].kind = 0;
    mutate_during_walk = 1;
    func_8013E370();
    assert(searches == 3 && predicates == 3 && created == 2);
    assert(entry_ids[0] == 2 && slot_ids[0] == 4);
    assert(entry_ids[1] == 4 && slot_ids[1] == 3);
    assert(D_801202A0[4].used == 3 && D_801202A0[3].used == 5);
    assert(D_801202A0[SLOT_COUNT - 1].used == 0);
    for (unsigned int i = 0; i < ENTRY_COUNT; ++i) {
        int marked = i == 0 || i == 2 || i == 3 || i == 4;
        assert(entries[i].flags == ((i + 10) | (marked ? 0x8000u : 0)));
    }
}

int main(void)
{
    test_distance_band();
    test_first_free_slot();
    test_empty_and_full();
    test_binding_order();
    return 0;
}

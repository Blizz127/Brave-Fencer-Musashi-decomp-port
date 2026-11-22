#include "actor_pickup.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

enum { ACTORS = 96, REJECTED = 4, ITEM = ACTORS - 1 };
_Static_assert(sizeof(Obj8014A2E4) == 0x10c, "actor stride");
Obj8014A2E4 D_801202A0[ACTORS];
u16 D_80078EB4;
s32 func_801505FC(void *self);

static u8 player[32];
static const u16 initial_flags = 0x9b40;
static u16 kind, injected[3][2], expected_value;
static s16 first_index, second_index;
static unsigned failures_at, reads, contacts;
static int contact_enabled;
static char history[16];
static size_t used;

static void record(char event)
{
    assert(used + 1 < sizeof history);
    history[used++] = event;
    history[used] = 0;
}

static void index_of_item(s16 index)
{
    u8 *item = (u8 *)&D_801202A0[ITEM];
    item[0x70] = (u8)(u16)index;
    item[0x71] = (u8)((u16)index >> 8);
}

int func_8014C278(char *source, char *target, int limit)
{
    assert(source == (char *)player && limit == 0x30);
    assert(target == (char *)&D_801202A0[contacts ? ITEM : REJECTED]);
    ++contacts;
    return contact_enabled && target == (char *)&D_801202A0[ITEM];
}

s32 func_80147324(u16 sound)
{
    assert(sound == 1106);
    assert(D_801202A0[ITEM].flags == (initial_flags | 1));
    record('s');
    index_of_item(first_index);
    D_801202A0[ITEM].active = 7;
    D_80078EB4 = 0;
    return -49;
}

static unsigned expected_table(void)
{
    if (kind == 49)
        return BFM_PICKUP_TABLE_49;
    if (kind == 50)
        return BFM_PICKUP_TABLE_50;
    assert(kind == 51);
    return BFM_PICKUP_TABLE_51;
}

int bfm_pickup_table_value(unsigned table, s16 index, u16 *value)
{
    assert(table == expected_table() && reads < 2);
    assert(index == (reads ? second_index : first_index));
    expected_value = injected[table][index == first_index ? 0 : 1];
    record('r');
    if (++reads == failures_at)
        return 0;
    *value = expected_value;
    return 1;
}

void bfm_pickup_table_refused(unsigned table, s16 index)
{
    assert(reads == failures_at && table == expected_table());
    assert(index == (reads == 1 ? first_index : second_index));
    record('!');
}

static void apply(void *self, s32 value, char event)
{
    assert(self == player && value == expected_value && value >= 0x8000);
    record(event);
    index_of_item(second_index);
    /* Even if the key stays the same, the second lookup must read the new value. */
    injected[expected_table()][0] ^= 0x315bu;
}

s32 func_8014ADA8(void *self, s32 value)
{
    assert(kind == 49);
    apply(self, value, 'a');
    return -57;
}

void func_8014BB24(void *self, s32 value, s32 mode)
{
    assert(kind == 50 && mode == 1);
    apply(self, value, 'b');
}

s32 func_8014ACE8(char *self, s32 mode, s32 value)
{
    assert(self == (char *)player && mode == (kind == 49 ? 3 : 2));
    assert(value == expected_value && value >= 0x8000);
    record('c');
    return -73;
}

void func_80150820(void *self, s32 value)
{
    assert(kind == 51 && self == player && value == expected_value && value >= 0x8000);
    record('d');
}

s32 func_8015086C(void *self)
{
    assert((kind == 197 || kind == 635) && self == player);
    record('t');
    return -89;
}

static void pickup(u16 type, u16 gate, int available, unsigned missing,
                   s16 first, s16 second, const char *events)
{
    memset(D_801202A0, 0, sizeof D_801202A0);
    D_801202A0[0].flags = initial_flags;
    D_801202A0[1].active = type;
    D_801202A0[REJECTED].active = type;
    D_801202A0[REJECTED].flags = initial_flags;
    D_801202A0[ITEM].active = type;
    D_801202A0[ITEM].flags = initial_flags;
    kind = type;
    D_80078EB4 = gate;
    contact_enabled = available;
    reads = contacts = 0;
    failures_at = missing;
    first_index = first;
    second_index = second;
    index_of_item(83);
    used = 0;
    history[0] = 0;
    for (unsigned table = 0; table < 3; ++table) {
        injected[table][0] = (u16)(0xba07u + 53 * table);
        injected[table][1] = (u16)(0xe74fu - 79 * table);
    }

    assert(func_801505FC(player) == available);
    assert(contacts == 2 && strcmp(history, events) == 0);
    assert(D_801202A0[ITEM].flags == (u16)(initial_flags | (unsigned)available));
    assert(D_801202A0[REJECTED].flags == initial_flags);
}

int main(void)
{
    if ((uintptr_t)player <= UINT32_MAX || (uintptr_t)D_801202A0 <= UINT32_MAX)
        return 77;
    pickup(49, 0, 0, 0, -3, 29, "");
    pickup(49, 0, 1, 0, INT16_MIN, INT16_MAX, "srarc");
    pickup(49, 0, 1, 0, -13, -13, "srarc");
    pickup(50, 0, 1, 0, -7, 11, "");
    pickup(50, 0x8000, 1, 0, INT16_MAX, INT16_MIN, "srbrc");
    pickup(51, 0, 1, 0, -439, 0, "srd");
    pickup(197, 0, 1, 0, 0, 0, "st");
    pickup(635, 0, 1, 0, 0, 0, "st");
    pickup(0xfffe, 0, 1, 0, 0, 0, "");
    pickup(49, 1, 1, 1, -13, 8, "sr!");
    pickup(49, 1, 1, 2, 3, -25, "srar!");
    pickup(50, 1, 1, 1, 17, 9, "sr!");
    pickup(50, 1, 1, 2, -37, -19, "srbr!");
    pickup(51, 1, 1, 1, INT16_MIN, 0, "sr!");
    return 0;
}

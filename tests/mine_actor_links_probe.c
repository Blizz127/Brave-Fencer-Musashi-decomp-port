#include <assert.h>
#include <string.h>
#include "mine_actor_links.h"

enum { SLOT_SIZE = 0x58, SLOTS = 30, DATA_SIZE = 0x1400 };
static const u32 data_base = 0x184000;
static const u32 handler = 0x517204;
static u8 slots[SLOTS * SLOT_SIZE], data[DATA_SIZE], table_word[4];
static unsigned callbacks, stores;
static u32 reject_write, expected_slot, future_kind;
static int alter_slots;

static u8 *resolve(u32 address, unsigned width)
{
    u32 offset = address - BFM_MINE_LINK_SLOTS;
    if (offset <= sizeof(slots) - width)
        return slots + offset;
    offset = address - data_base;
    if (offset <= sizeof(data) - width)
        return data + offset;
    offset = address - BFM_MINE_LINK_TABLE_POINTER;
    if (offset <= sizeof(table_word) - width)
        return table_word + offset;
    return NULL;
}

static u32 get(u32 address, unsigned width)
{
    u8 *p = resolve(address, width);
    u32 result = 0;
    unsigned i;
    assert(p);
    for (i = 0; i < width; ++i)
        result |= (u32)p[i] << (8u * i);
    return result;
}

static void put(u32 address, unsigned width, u32 value)
{
    u8 *p = resolve(address, width);
    unsigned i;
    assert(p);
    for (i = 0; i < width; ++i)
        p[i] = (u8)(value >> (8u * i));
}

int bfm_mine_link_read_half(u32 address, u16 *value)
{
    assert(!(address & 1u));
    if (!resolve(address, 2))
        return 0;
    *value = (u16)get(address, 2);
    return 1;
}

int bfm_mine_link_read_word(u32 address, u32 *value)
{
    assert(!(address & 3u));
    if (!resolve(address, 4))
        return 0;
    *value = get(address, 4);
    return 1;
}

int bfm_mine_link_store_half(u32 address, u16 value)
{
    assert(!(address & 1u));
    if (address == reject_write || !resolve(address, 2))
        return 0;
    ++stores;
    put(address, 2, value);
    return 1;
}

int bfm_mine_link_store_word(u32 address, u32 value)
{
    assert(!(address & 3u));
    if (address == reject_write || !resolve(address, 4))
        return 0;
    ++stores;
    put(address, 4, value);
    return 1;
}

int bfm_mine_link_run_handler(u32 selected, u32 slot)
{
    assert(stores == 0 && selected == handler);
    assert(slot == expected_slot);
    ++callbacks;
    if (alter_slots && callbacks == 1) {
        /* A handler can replace both a future type and the table it will use. */
        expected_slot += (SLOTS - 1) * SLOT_SIZE;
        put(expected_slot, 2, future_kind);
        put(BFM_MINE_LINK_TABLE_POINTER, 4, data_base + 0xe0);
        put(data_base + 0xe0 + 4 * future_kind, 4, handler);
    }
    return 1;
}

static void reset(void)
{
    memset(slots, 0, sizeof(slots));
    memset(data, 0, sizeof(data));
    memset(table_word, 0, sizeof(table_word));
    callbacks = stores = 0;
    reject_write = expected_slot = future_kind = 0;
    alter_slots = 0;
}

static void coordinates(u32 slot)
{
    put(slot + 6, 2, 0x8047);
    put(slot + 10, 2, 0x735a);
    put(slot + 14, 2, 0xe2b5);
}

static void check_direct(unsigned index)
{
    u32 slot = BFM_MINE_LINK_SLOTS + index * SLOT_SIZE;
    u32 target = data_base + 0x620;
    reset();
    coordinates(slot);
    put(slot + 0x20, 4, target);
    put(target, 2, 1);
    put(target + 0x2c, 2, 0x1582);
    assert(bfm_mine_update_actor_links());
    assert(!callbacks && stores == 7);
    assert(get(target + 8, 2) == 0x8047 && get(target + 0x48, 4) == 0xffff8047);
    assert(get(target + 10, 2) == 0x735a && get(target + 0x4c, 4) == 0x735a);
    assert(get(target + 12, 2) == 0xe2b5 && get(target + 0x50, 4) == 0xffffe2b5);
    assert(get(target + 0x2c, 2) == 0x1583);
}

static void check_handlers(void)
{
    u32 first = BFM_MINE_LINK_SLOTS;
    u32 last = first + (SLOTS - 1) * SLOT_SIZE;
    reset();
    coordinates(last);
    put(first, 2, 19);
    put(BFM_MINE_LINK_TABLE_POINTER, 4, data_base);
    put(data_base + 19 * 4, 4, handler);
    put(last + 0x20, 4, data_base + 0x880);
    expected_slot = first;
    future_kind = 53;
    alter_slots = 1;
    func_80165CA0();
    assert(callbacks == 2 && stores == 3);
    assert(get(data_base + 0x88c, 2) == 0xe2b5);
}

static void check_alias_and_refusals(void)
{
    u32 slot = BFM_MINE_LINK_SLOTS;
    u32 target = data_base + 0x500;
    u32 nested = target + 0x20;
    reset();
    coordinates(slot);
    put(slot + 0x20, 4, target);
    put(target + 0x34, 4, nested);
    /* Writing nested X replaces target's subobject handle before Y and Z. */
    put(slot + 6, 2, 0x2400);
    assert(!bfm_mine_update_actor_links());
    assert(stores == 3 && get(target + 0x34, 4) == 0x2400);
    assert(get(target + 10, 2) == 0x735a && get(target + 12, 2) == 0);
    reset();
    coordinates(slot);
    put(slot + 0x20, 4, BFM_MINE_LINK_SLOTS + 4);
    /* The Y write changes the source halfword read for Z. */
    assert(bfm_mine_update_actor_links());
    assert(stores == 3 && get(slot + 12, 2) == 0x8047 && get(slot + 16, 2) == 0x735a);
    reset();
    coordinates(slot);
    put(slot + 0x20, 4, target);
    put(target, 2, 1);
    reject_write = target + 0x2c;
    assert(!bfm_mine_update_actor_links());
    assert(stores == 5 && get(target + 12, 2) == 0xe2b5 && get(target + 0x50, 4) == 0);
    reset();
    coordinates(slot);
    put(slot + 0x20, 4, target);
    put(target, 2, 19);
    put(target + 0x34, 4, data_base + 0x800);
    assert(bfm_mine_update_actor_links() && stores == 6);
    assert(get(data_base + 0x814, 4) == 0xffff8047 && get(data_base + 0x81c, 4) == 0xffffe2b5);
    reset();
    put(slot, 2, 1);
    put(BFM_MINE_LINK_TABLE_POINTER, 4, data_base + 1);
    assert(!bfm_mine_update_actor_links() && !callbacks && !stores);
}

int main(void)
{
    unsigned i;
    for (i = 0; i < SLOTS; ++i)
        check_direct(i);
    check_handlers();
    check_alias_and_refusals();
    return 0;
}

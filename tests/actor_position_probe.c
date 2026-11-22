#include "actor_position_updates.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(Obj8014A2E4) == 0x10c, "actor stride");
_Static_assert(offsetof(Obj8014A2E4, probe_object_address) == 0x20, "actor child key");

enum {
    COUNT = 96, FIRST_TABLE = 0x3420, NEXT_TABLE = 0x4764,
    FIRST_ACTION = 0x5878, LAST_ACTION = 0x6394, UNKNOWN_ACTION = 0x6924,
    FIRST_CHILD = 0x8bd0, NEXT_CHILD = 0xa628
};
enum Scenario { ORDINARY, NO_FINISH, BAD_TABLE, ZERO_KEY, UNKNOWN_KEY, SHORT_CHILD };

Obj8014A2E4 D_801202A0[COUNT];
s16 D_801270C0;
u32 D_8011DB08;
static enum Scenario scenario;
static unsigned prologue, prepared, first_calls, last_calls, alternate_calls, finished;
static unsigned lookups, child_writes, refusals, refusal, detail;
static u32 refused_key;
static u8 first_child[16], next_child[16];

static u32 load_le(const void *record, unsigned offset, unsigned size)
{
    const u8 *bytes = record;
    u32 value = 0;
    for (unsigned i = 0; i < size; ++i)
        value |= (u32)bytes[offset + i] << (8 * i);
    return value;
}

static void store_le(void *record, unsigned offset, unsigned size, u32 value)
{
    u8 *bytes = record;
    for (unsigned i = 0; i < size; ++i)
        bytes[offset + i] = (u8)(value >> (8 * i));
}

static u32 prepared_coordinate(unsigned index, unsigned axis)
{
    return 0x25687d91u + index * 0x5137u + axis * 0x607215u;
}

static void check_history(Obj8014A2E4 *actor)
{
    unsigned index = (unsigned)(actor - D_801202A0);
    for (unsigned axis = 0; axis < 3; ++axis)
        assert(load_le(actor, 0x38 + 4 * axis, 4) == prepared_coordinate(index, axis));
}

void func_801787D4(void)
{
    assert(!prologue && !prepared);
    ++prologue;
}

void func_8012A908(void)
{
    assert(prologue == 1 && !prepared);
    ++prologue;
}

void func_8017869C(Obj8014A2E4 *actor)
{
    assert(prologue == 2);
    ++prepared;
    unsigned index = (unsigned)(actor - D_801202A0);
    for (unsigned axis = 0; axis < 3; ++axis)
        store_le(actor, 4 + 4 * axis, 4, prepared_coordinate(index, axis));
    if (scenario >= BAD_TABLE)
        return;
    if (index == 1) {
        assert(prepared == 1);
        actor->active = 2;
        D_8011DB08 = FIRST_TABLE;
        ((u8 *)actor)[0xc0] = 0xff;
    } else if (index == 2) {
        assert(prepared == 2 && first_calls == 1);
        ((u8 *)actor)[0xc0] = 0x81;
        ((u8 *)actor)[0xc1] = 0xf2;
    } else {
        assert(index == COUNT - 1 && prepared == 3);
        ((u8 *)actor)[0xc1] = 0xff;
    }
}

static void first(Obj8014A2E4 *actor)
{
    check_history(actor);
    ++first_calls;
    if (scenario >= BAD_TABLE) {
        actor->probe_object_address = FIRST_CHILD;
        return;
    }
    assert(actor == &D_801202A0[1] && actor->active == 2);
    D_8011DB08 = NEXT_TABLE;
    D_801270C0 = 1;
    actor->probe_object_address = FIRST_CHILD;
    store_le(actor, 4, 4, 0x7ffe9561);
    store_le(actor, 8, 4, 0x80006aeb);
    store_le(actor, 12, 4, 0xffed4b21);
    store_le(actor, 0x50, 2, 3);
    store_le(actor, 0x52, 2, (u16)-1);
    store_le(actor, 0x54, 2, 0x34);
}

s32 func_80130D0C(Obj8014A2E4 *actor)
{
    assert(actor == &D_801202A0[2] && prepared == 2);
    check_history(actor);
    ++alternate_calls;
    D_801270C0 = -3;
    actor->probe_object_address = 0;
    return -53;
}

static void last(Obj8014A2E4 *actor)
{
    assert(actor == &D_801202A0[COUNT - 1] && alternate_calls == 1);
    check_history(actor);
    ++last_calls;
    D_801270C0 = scenario == ORDINARY ? 1 : -1;
}

void func_8013E370(void)
{
    assert(last_calls == 1 && D_801270C0 == 1);
    ++finished;
}

int bfm_actor_update_handler_key(u32 table, u16 type, u32 *out)
{
    ++lookups;
    if (scenario >= BAD_TABLE) {
        assert(type == 7 && table == FIRST_TABLE);
        if (scenario == BAD_TABLE)
            return 0;
        *out = scenario == ZERO_KEY ? 0 : scenario == UNKNOWN_KEY ? UNKNOWN_ACTION : FIRST_ACTION;
    } else if (table == FIRST_TABLE) {
        assert(type == 2 && lookups == 1);
        *out = FIRST_ACTION;
    } else {
        assert(table == NEXT_TABLE && type == 9 && lookups == 2);
        *out = LAST_ACTION;
    }
    return 1;
}

SceneActorUpdateHandler bfm_actor_update_handler(u32 key)
{
    if (key == FIRST_ACTION)
        return first;
    if (key == LAST_ACTION)
        return last;
    return NULL;
}

u8 *bfm_actor_update_child_span(u32 key, unsigned offset, unsigned width)
{
    assert(width == 2);
    assert(offset == 8 + 2 * child_writes);
    ++child_writes;
    if (offset == 8) {
        assert(key == FIRST_CHILD);
        if (scenario != SHORT_CHILD)
            D_801202A0[1].probe_object_address = NEXT_CHILD;
        return &first_child[offset];
    }
    if (scenario == SHORT_CHILD) {
        assert(offset == 10 && key == FIRST_CHILD);
        return NULL;
    }
    assert(key == NEXT_CHILD);
    return &next_child[offset];
}

void bfm_actor_update_refused(unsigned reason, u32 key, unsigned offset)
{
    ++refusals;
    refusal = reason;
    refused_key = key;
    detail = offset;
}

static void reset(enum Scenario value)
{
    scenario = value;
    prologue = prepared = first_calls = last_calls = alternate_calls = finished = 0;
    lookups = child_writes = refusals = 0;
    memset(D_801202A0, 0, sizeof D_801202A0);
    memset(first_child, 0, sizeof first_child);
    memset(next_child, 0, sizeof next_child);
    D_801270C0 = 1;
    D_8011DB08 = FIRST_TABLE;
}

static void sweep(enum Scenario value)
{
    reset(value);
    D_801270C0 = 2;
    D_8011DB08 = NEXT_TABLE;
    D_801202A0[0].active = 7;
    D_801202A0[1].active = 5;
    D_801202A0[2].active = 13;
    D_801202A0[COUNT - 2].active = 11;
    D_801202A0[COUNT - 1].active = 9;
    store_le(&D_801202A0[1], 0x72, 2, 0x1000);
    store_le(&D_801202A0[COUNT - 1], 0x72, 2, 0x1400);
    D_801202A0[2].probe_object_address = FIRST_CHILD;

    func_8017849C();

    assert(prepared == 3 && first_calls == 1 && alternate_calls == 1 && last_calls == 1);
    assert(!refusals && lookups == 2 && child_writes == 3);
    assert(finished == (unsigned)(scenario == ORDINARY));
    assert(load_le(first_child, 8, 2) == 0x8001 && !load_le(first_child, 10, 2));
    assert(!load_le(next_child, 8, 2) && load_le(next_child, 10, 2) == 0x7fff);
    assert(load_le(next_child, 12, 2) == 0x21);
    for (unsigned i = 0; i < COUNT; ++i) {
        if (i == 1 || i == 2 || i == COUNT - 1)
            check_history(&D_801202A0[i]);
        else
            assert(!load_le(&D_801202A0[i], 0x38, 4));
    }
}

static void failed(enum Scenario value)
{
    reset(value);
    D_801202A0[0].active = 7;
    D_801202A0[1].active = 7;
    func_8017849C();
    check_history(&D_801202A0[0]);
    assert(refusals == 1 && prepared == 1 && !finished && lookups == 1);
    assert(!load_le(&D_801202A0[1], 0x38, 4));
    if (value == SHORT_CHILD) {
        assert(refusal == BFM_ACTOR_UPDATE_BAD_CHILD && refused_key == FIRST_CHILD && detail == 10);
        assert(first_calls == 1 && child_writes == 2);
        assert(load_le(first_child, 8, 2) == prepared_coordinate(0, 0) >> 16);
    } else if (value == BAD_TABLE) {
        assert(refusal == BFM_ACTOR_UPDATE_BAD_TABLE && refused_key == FIRST_TABLE && detail == 7);
    } else {
        assert(refusal == BFM_ACTOR_UPDATE_BAD_HANDLER && detail == 7);
        assert(refused_key == (value == ZERO_KEY ? 0 : UNKNOWN_ACTION));
    }
}

int main(void)
{
    if ((uintptr_t)D_801202A0 <= UINT32_MAX || (uintptr_t)first_child <= UINT32_MAX ||
        (uintptr_t)next_child <= UINT32_MAX)
        return 77;
    sweep(ORDINARY);
    sweep(NO_FINISH);
    failed(BAD_TABLE);
    failed(ZERO_KEY);
    failed(UNKNOWN_KEY);
    failed(SHORT_CHILD);
    return 0;
}

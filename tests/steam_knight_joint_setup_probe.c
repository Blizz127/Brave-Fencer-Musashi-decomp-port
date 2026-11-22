#include "steam_knight_hit.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

void func_8018968C(SteamKnightPart *part);

enum { PART = 0x1400, SLOTS = 0x2300, MODEL = 0x3200, RESOURCE = 0x4100, CHILD = 0x5000 };
static SteamKnightPart actor;
static u8 resources[4][64], children[4][40], models[2][32];
static u32 slots[10];
static s16 placement[3][4];
static const char *events;
static unsigned allocated, mask;
static int slot, model_ok, redirect;

static void step(char c) { assert(*events == c); ++events; }
static u32 word(const u8 *p) { u32 n; memcpy(&n, p, sizeof n); return n; }
static s16 half(const u8 *p) { s16 n; memcpy(&n, p, sizeof n); return n; }
static void put_word(u8 *p, u32 n) { memcpy(p, &n, sizeof n); }

void *bfm_tower_spawn_pointer(u32 key)
{
    if (key >= SLOTS && key - SLOTS < sizeof slots && !(key & 3))
        return (u8 *)slots + (key - SLOTS);
    if (key >= 0x801965dcu && key - 0x801965dcu < sizeof placement && !(key & 1))
        return (u8 *)placement + (key - 0x801965dcu);
    for (unsigned i = 0; i < 4; ++i) {
        if (key == RESOURCE + i) return resources[i];
        if (key == CHILD + i) return children[i];
    }
    for (unsigned i = 0; i < 2; ++i)
        if (key == MODEL + i) return models[i];
    assert(0);
    return NULL;
}

u32 bfm_tower_spawn_address(const void *p)
{
    if (p == &actor) return PART;
    if (p == models[0]) return MODEL;
    for (unsigned i = 0; i < 4; ++i)
        if (p == resources[i]) return RESOURCE + i;
    assert(0);
    return 0;
}

u32 bfm_tower_joint_resource_slots(void) { return SLOTS; }

s32 func_8018B300(void)
{
    step('S');
    return 65536 + slot;
}

u8 *func_8012913C(u16 kind)
{
    unsigned i = allocated++;
    step('R');
    assert(kind == 9 && i < 4);
    return mask & (1u << i) ? resources[i] : NULL;
}

void *func_8012C1B8(void)
{
    step('M');
    assert(word((u8 *)&actor + 4) == 0 && word((u8 *)&actor + 8) == 0);
    assert(word((u8 *)&actor + 12) == 0 && actor.variant == 217);
    return model_ok ? models[0] : NULL;
}

void func_801292C8(char *p)
{
    for (unsigned i = 0; i < 4; ++i) {
        if ((void *)p == resources[i]) {
            step((char)('a' + i));
            return;
        }
    }
    assert(0);
}

s32 func_8012CAE4(u8 *p)
{
    step('D');
    assert(p == (u8 *)&actor && actor.model_address == (model_ok ? MODEL : 0));
    actor.active = 0;
    return -3;
}

void bfm_tower_armor_bind_model(void *p)
{
    step('B');
    assert(p == models[0] && actor.model_address == MODEL);
    if (redirect) {
        actor.model_address = MODEL + 1;
        actor.break_delay = 2;
    }
}

static void check(int selector, int selected_slot, unsigned available, int has_model,
                  int mutate, s16 saved_count, const char *calls)
{
    memset(&actor, 0x55, sizeof actor);
    memset(resources, 0xe3, sizeof resources);
    memset(children, 0x26, sizeof children);
    memset(models, 0x31, sizeof models);
    for (unsigned i = 0; i < 10; ++i) slots[i] = i % 2 ? 0 : RESOURCE + ((i + 2) % 4);
    for (unsigned i = 0; i < 4; ++i)
        put_word(resources[i] + 0x20, CHILD + i);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            placement[i][j] = (s16)(i * 51 - 75 + j * 11);
    actor.active = 1;
    actor.state = 65535;
    actor.field_fc = 147;
    actor.variant = (s16)((selector << 8) | 217);
    actor.break_count = (u16)saved_count;
    events = calls; slot = selected_slot; mask = available;
    model_ok = has_model; redirect = mutate; allocated = 0;
    func_8018968C(&actor);
    assert(*events == '\0' && half((u8 *)&actor + 0x102) == slot);
    unsigned n = slot < 0 ? 0u : selector < 2 ? 4u : 1u;
    assert(allocated == n && actor.break_count == (slot < 0 ? (u16)saved_count : n));
    for (unsigned i = 0; i < 10; ++i) {
        u32 old = i % 2 ? 0 : RESOURCE + ((i + 2) % 4);
        assert(slots[i] == (i < n ? (mask & (1u << i) ? RESOURCE + i : 0) : old));
    }
    int ok = n && mask == (1u << n) - 1 && has_model;
    assert(actor.active == ok && actor.state == (ok ? 0 : 65535));
    assert(actor.field_fc == (ok ? 0 : 147));
    assert(actor.break_delay == (redirect ? 2 : selector));
    for (unsigned i = 0; i < 4; ++i) {
        int made = i < n && (mask & (1u << i));
        assert(half(children[i] + 0x1e) == (made ? (selector >= 2) : 0x2626));
        assert(word(resources[i] + 0x2c) == (ok && made ? (i ? 0u : PART) : 0xe3e3e3e3u));
        u32 block = 0x801e44f8u + (u32)slot * 128u + i * 32u;
        assert(word(resources[i] + 0x34) == (ok && made ? block : 0xe3e3e3e3u));
    }
    assert(actor.model_address == (has_model ? MODEL + (u32)redirect : 0));
    assert(half((u8 *)&actor + 6) == (ok ? placement[actor.break_delay][0] : 0));
    assert(actor.position_y == (ok ? placement[actor.break_delay][1] : 0));
    for (int i = 0; i < 2; ++i) {
        assert(half(models[i] + 16) == (ok && i == redirect ? placement[actor.break_delay][2] : 0x3131));
        assert(half(models[i] + 18) == (ok && i == redirect ? placement[actor.break_delay][3] : 0x3131));
    }
}

int main(void)
{
    check(1, 5, 15, 1, 1, 0, "SRRRRMB");
    check(2, 3, 1, 1, 0, 0, "SRMB");
    check(0, 0, 5, 1, 0, 2, "SRRRRMacD");
    check(1, 2, 15, 0, 0, 0, "SRRRRMabcdD");
    check(2, 1, 0, 0, 0, 4, "SRMD");
    check(1, -1, 0, 1, 0, 3, "SMcaD");
    check(0, -7, 0, 0, 0, -2, "SMD");
    return 0;
}

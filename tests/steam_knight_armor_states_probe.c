#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "steam_knight_hit.h"

void func_8018874C(SteamKnightPart *part);
void func_801887CC(SteamKnightPart *part);
s32 D_801E43A0;

/* Distinct serialized identities keep the selected host actor observable. */
static const u32 actor_key[] = {0x8abcf010u, 0xa1e23060u};
static const u32 phase_bit = 0x8000u;
static const u32 initial_flags = 0x30114054u;

typedef struct {
    SteamKnightPart part, owners[2];
} Actors;

enum Event { BREAK, DELAY_EFFECT, RESOLVE, PHASE_EFFECT };
static Actors *actors;
static enum Event events[3];
static unsigned event_count, redirect_owner;
static u32 resolved_key;
static s16 expected_delay;

static void record(enum Event event)
{
    assert(event_count < sizeof events / sizeof events[0]);
    events[event_count++] = event;
}

static void change_owner(SteamKnightPart *part)
{
    assert(part == &actors->part);
    assert(part->linked_address == actor_key[0]);
    assert(part->break_delay == expected_delay);
    part->linked_address = actor_key[redirect_owner];
    actors->owners[redirect_owner].phase_flags ^= phase_bit;
}

void func_80185060(SteamKnightPart *part, int mode)
{
    assert(mode == 0);
    record(BREAK);
    change_owner(part);
}

void func_801870BC(SteamKnightPart *part)
{
    record(DELAY_EFFECT);
    change_owner(part);
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 key)
{
    record(RESOLVE);
    resolved_key = key;
    assert(key == actor_key[0] || key == actor_key[1]);
    return &actors->owners[key == actor_key[1]];
}

void func_80185214(SteamKnightPart *part)
{
    assert(part == &actors->part);
    assert(part->break_delay == expected_delay);
    record(PHASE_EFFECT);
    part->reaction = 0x73;
}

static void check(int active, int value, int selected_bit, unsigned phase, unsigned redirect)
{
    Actors input, expected;
    unsigned index = 0;
    int child;
    u32 mask = UINT32_C(1) << ((unsigned)(value + 1) % 32u);
    if (!selected_bit) mask = ~mask;
    memcpy(&D_801E43A0, &mask, sizeof mask);

    memset(&input, 0x56, sizeof input);
    input.part.variant = active ? (s16)value : 7;
    input.part.break_delay = active ? 203 : (s16)value;
    input.part.linked_address = actor_key[0];
    input.owners[0].phase_flags = initial_flags | (phase ? phase_bit : 0);
    input.owners[1].phase_flags = input.owners[0].phase_flags;
    memcpy(&expected, &input, sizeof expected);

    child = active ? (value < 13 && selected_bit) : value != 0;
    if (!active && child)
        expected.part.break_delay = value == INT16_MIN ? INT16_MAX : (s16)(value - 1);
    if (child) {
        expected.part.linked_address = actor_key[redirect];
        expected.owners[redirect].phase_flags ^= phase_bit;
    }
    if (phase != (unsigned)child) expected.part.reaction = 0x73;
    expected_delay = expected.part.break_delay;
    redirect_owner = redirect;
    actors = &input;
    event_count = 0;
    resolved_key = 0;
#if UINTPTR_MAX > UINT32_MAX
    assert((uintptr_t)&input.part > UINT32_MAX && (uintptr_t)&input.owners[1] > UINT32_MAX);
#endif
    if (active) func_8018874C(&input.part);
    else func_801887CC(&input.part);

    if (child) assert(events[index++] == (active ? BREAK : DELAY_EFFECT));
    assert(events[index++] == RESOLVE);
    assert(resolved_key == expected.part.linked_address);
    if (phase != (unsigned)child) assert(events[index++] == PHASE_EFFECT);
    assert(event_count == index);
    assert(!memcmp(&input, &expected, sizeof expected));
    u32 after_mask;
    memcpy(&after_mask, &D_801E43A0, sizeof after_mask);
    assert(after_mask == mask);
}

int main(void)
{
    for (int value = INT16_MIN; value <= INT16_MAX; ++value) {
        for (unsigned phase = 0; phase < 2; ++phase) {
            for (unsigned redirect = 0; redirect < 2; ++redirect) {
                check(1, value, 0, phase, redirect);
                check(1, value, 1, phase, redirect);
                check(0, value, 0, phase, redirect);
            }
        }
    }
    return 0;
}

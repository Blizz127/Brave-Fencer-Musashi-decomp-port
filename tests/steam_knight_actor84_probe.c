#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "steam_knight_actor84.h"

_Static_assert(offsetof(SteamKnightPart, state) == 2, "state offset");
_Static_assert(offsetof(SteamKnightPart, timer) == 28, "timer offset");
_Static_assert(offsetof(SteamKnightPart, model_address) == 32, "model offset");
_Static_assert(offsetof(SteamKnightPart, variant) == 112, "variant offset");
_Static_assert(offsetof(SteamKnightPart, field_fc) == 252, "first angle offset");
_Static_assert(offsetof(SteamKnightPart, break_count) == 254, "second angle offset");

enum { SETUP = 0x22344170, OTHER = 0x2234419c, HELD = 0x23450028 };
static SteamKnightPart actor;
static u32 table[65536], expected_target, returned_model, expected_list;
static s16 variant_after_alloc;
static u16 state_in_second_rng;
static unsigned lookups, callbacks, calls;
static char sequence[7];

void *bfm_tower_spawn_pointer(u32 address)
{
    assert(address == 0x801966b8u + 4u * actor.state);
    ++lookups;
    return &table[actor.state];
}

void bfm_tower_spawn_setup(u32 target, void *arg)
{
    assert(arg == &actor && target == expected_target);
    ++callbacks;
    if (target == SETUP)
        func_8018BA38(&actor);
}

static void service(char symbol)
{
    assert(calls + 1 < sizeof sequence);
    sequence[calls++] = symbol;
    sequence[calls] = '\0';
}

u32 bfm_tower_actor84_allocate_model(void)
{
    service('A');
    assert(actor.model_address == 0xd111e4a8u);
    actor.variant = variant_after_alloc;
    return returned_model;
}

void bfm_tower_actor84_dispose(SteamKnightPart *part)
{
    service('D');
    assert(part == &actor && part->model_address == 0);
    part->state = 27;
    part->timer = -15;
}

void bfm_tower_actor84_model_list(u32 model, u32 list)
{
    service('M');
    assert(model == returned_model && actor.model_address == model);
    assert(list == expected_list);
    actor.model_address = 0xc145a29cu;
}

void bfm_tower_actor84_animation(SteamKnightPart *part, u32 script)
{
    service('S');
    assert(part == &actor && part->model_address == 0xc145a29cu);
    assert(script == 0x80190c0cu);
    part->state = 13;
}

s32 func_8005C49C(void)
{
    service('R');
    if (calls == 4) {
        assert(actor.state == 13 && actor.field_fc == 0x529a);
        return 0x1c7b6ab5;
    }
    assert(calls == 5 && actor.field_fc == 0x2b0 && actor.break_count == 0xe598);
    actor.state = state_in_second_rng;
    return -23;
}

static void dispatch(u16 state, u32 target)
{
    unsigned reads = lookups, invoked = callbacks;
    actor.state = state;
    table[state] = target;
    expected_target = target;
    func_80189CCC(&actor);
    assert(lookups == reads + 1 && callbacks == invoked + 1);
}

static void check_dispatch(void)
{
    lookups = callbacks = calls = 0;
    sequence[0] = '\0';
    /* One slot changes target between calls. The unhandled target is forwarded. */
    dispatch(0, HELD);
    dispatch(1, OTHER);
    dispatch(65535, HELD);
    dispatch(65535, OTHER);
    assert(calls == 0);
}

static void check_setup(u32 model, s16 new_variant, u32 list, u16 last_state)
{
    memset(&actor, 0x75, sizeof actor);
    actor.variant = new_variant ^ 1;
    actor.field_fc = 0x529a;
    actor.break_count = 0xe598;
    actor.model_address = 0xd111e4a8u;
    returned_model = model;
    variant_after_alloc = new_variant;
    expected_list = list;
    state_in_second_rng = last_state;
    calls = lookups = callbacks = 0;
    sequence[0] = '\0';

    SteamKnightPart expected;
    memcpy(&expected, &actor, sizeof expected);
    expected.variant = new_variant;
    dispatch(0, SETUP);
    if (model == 0) {
        assert(strcmp(sequence, "AD") == 0);
        expected.model_address = 0;
        expected.state = 27;
        expected.timer = -15;
    } else {
        assert(strcmp(sequence, "AMSRR") == 0);
        expected.model_address = 0xc145a29cu;
        expected.state = (u16)(last_state + 1u);
        expected.timer = 96;
        expected.field_fc = 0x2b0;
        expected.break_count = 0x3e0;
    }
    assert(memcmp(&actor, &expected, sizeof actor) == 0);
}

int main(void)
{
    check_dispatch();
    check_setup(0, 2, 0, 0);
    check_setup(0xdf6248f0u, 6, 0x801966a8u, 57);
    check_setup(0xd8212718u, -3, 0x801966b0u, 65535);
    return 0;
}

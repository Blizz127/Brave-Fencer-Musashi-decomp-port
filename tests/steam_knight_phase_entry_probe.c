#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "steam_knight_hit.h"

extern void func_80182998(SteamKnightPart *body);
extern void func_8012AD44(u8 *actor, u16 state);

_Static_assert(offsetof(SteamKnightPart, state) == 2, "state");
_Static_assert(offsetof(SteamKnightPart, secondary_address) == 0xd0, "secondary handle");
_Static_assert(offsetof(SteamKnightPart, break_cause) == 0x104, "saved timer");

enum { SUBSTATE = 0x34, MODEL_VALUE = 0x12 };
#define LINKED 0xd2143000u
#define MODEL 0xac581000u
#define SECONDARY 0x96307240u
#define SECONDARY_AFTER_CLEAR 0x7240u

typedef struct {
    u32 guard[4];
    SteamKnightPart body, linked, secondary, alternate;
    u8 model[48];
    u32 trailer[4];
} Fixture;

typedef struct {
    u32 final_phase;
    s32 final_timer;
    s16 saved_timer, final_substate;
    int animation, alias_model;
} Scenario;

static Fixture live, expected;
static Scenario scenario;
static const char *pending;

static void store_half(void *base, size_t offset, u16 value)
{
    memcpy((u8 *)base + offset, &value, sizeof value);
}

static void step(char name)
{
    assert(*pending == name);
    ++pending;
    assert(memcmp(&live, &expected, sizeof live) == 0);
}

static u8 *model_data(Fixture *f)
{
    /* Clearing this model halfword changes the secondary handle's high half. */
    if (scenario.alias_model)
        return (u8 *)&f->body + offsetof(SteamKnightPart, secondary_address) + 2 - MODEL_VALUE;
    return f->model;
}

void *bfm_tower_spawn_pointer(u32 address)
{
    if (address - LINKED < sizeof live.linked)
        return (u8 *)&live.linked + (address - LINKED);
    if (address - MODEL < sizeof live.model)
        return model_data(&live) + (address - MODEL);
    if (address == SECONDARY)
        return &live.secondary;
    if (address == SECONDARY_AFTER_CLEAR)
        return &live.alternate;
    assert(0 && "unexpected actor or model handle");
    return NULL;
}

void func_8012B23C(s32 *body)
{
    assert((void *)body == (void *)&live.body);
    step('P');
    live.body.phase_flags = expected.body.phase_flags = 0xdb051473u;
}

void func_80186F58(SteamKnightPart *body)
{
    assert(body == &live.body);
    expected.body.phase_flags &= ~1u;
    step('R');
    body->linked_address = expected.body.linked_address = LINKED;
    body->secondary_address = expected.body.secondary_address = SECONDARY;
    live.linked.model_address = expected.linked.model_address = MODEL;
    body->phase_flags = expected.body.phase_flags = scenario.final_phase ^ 0x61000u;
}

void func_80184C20(SteamKnightPart *secondary)
{
    store_half(model_data(&expected), MODEL_VALUE, 0);
    assert(secondary == (scenario.alias_model ? &live.alternate : &live.secondary));
    step('C');
    secondary->state = 0xe2a8;
    (scenario.alias_model ? &expected.alternate : &expected.secondary)->state = 0xe2a8;
    live.body.timer = expected.body.timer = 0x34b2;
}

void func_8018AAAC(void)
{
    step('G');
    live.body.phase_flags = expected.body.phase_flags = scenario.final_phase;
    live.body.timer = expected.body.timer = scenario.final_timer;
    live.body.state = expected.body.state = 0x98a7;
    store_half(&live.body, SUBSTATE, 0x64a7);
    store_half(&expected.body, SUBSTATE, 0x64a7);
}

void func_80186304(SteamKnightPart *body, int animation, int mode)
{
    expected.body.state = 11;
    store_half(&expected.body, SUBSTATE, 0);
    expected.body.break_cause = scenario.saved_timer;
    assert(body == &live.body && animation == scenario.animation && mode == 2);
    step('A');
    /* The caller's last write must still use the branch taken before this call. */
    body->phase_flags = expected.body.phase_flags = scenario.final_phase ^ 0x61000u;
    body->state = expected.body.state = 0xdf7au;
    body->break_cause = expected.body.break_cause = -3143;
    store_half(body, SUBSTATE, 0xbe52);
    store_half(&expected.body, SUBSTATE, 0xbe52);
}

static void check_phase(Scenario input)
{
    scenario = input;
    pending = "PRCGA";
    memset(&live, 0xac, sizeof live);
    live.body.phase_flags = 0x10200u;
    live.body.break_cause = 6813;
    memcpy(&expected, &live, sizeof live);
    func_80182998(&live.body);
    store_half(&expected.body, SUBSTATE, (u16)input.final_substate);
    assert(*pending == '\0' && memcmp(&live, &expected, sizeof live) == 0);
}

static void check_setter(void)
{
    memset(&live, 0x59, sizeof live);
    memcpy(&expected, &live, sizeof live);
    for (u32 value = 0; value <= 0xffffu; ++value) {
        store_half(&live.body, SUBSTATE, 0xa531);
        expected.body.state = (u16)value;
        store_half(&expected.body, SUBSTATE, 0);
        func_8012AD44((u8 *)&live.body, (u16)value);
        assert(memcmp(&live, &expected, sizeof live) == 0);
    }
}

int main(void)
{
    check_setter();
    /* phase, timer, expected saved half, substate, animation, aliased model */
    check_phase((Scenario){0x84061001u, 0x21b8ff20, -224, 2, 25, 1});
    check_phase((Scenario){0x24020001u, 0x33847129, 6813, 0, 21, 0});
    check_phase((Scenario){0x20041000u, -82882, -17346, 1, 23, 0});
    check_phase((Scenario){0x20000008u, 0x42516, 6813, 1, 23, 1});
    return 0;
}

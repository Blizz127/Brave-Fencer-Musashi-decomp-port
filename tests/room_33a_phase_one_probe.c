#include "actor_33a.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(Actor33A) == 268, "actor stride");
_Static_assert(offsetof(Actor33A, gate.parameter) == 0x70, "signed parameter");
_Static_assert(sizeof(((Actor33A *)0)->gate.parameter) == 2, "parameter width");
_Static_assert(offsetof(Actor33A, state.cnt) == 2, "phase");
_Static_assert(offsetof(Actor33A, state.clr) == 0x34, "phase timer");
_Static_assert(_Generic(((Actor33A *)0)->gate.parameter, s16: 1, default: 0),
               "parameter must retain its signed width");

Actor33APhase D_80186344[8];
static Actor33A slots[4];
static Actor33A expected[4];
static unsigned index_under_test;
static unsigned calls;
static int provider_edits;

void func_8012AD44(u8 *actor, u16 phase)
{
    Actor33A *selected = &slots[index_under_test];
    Actor33A *wanted = &expected[index_under_test];
    assert(calls++ == 0);
    assert(actor == (u8 *)selected && phase == 6);
    assert(wanted->gate.parameter != 0);
    assert(memcmp(slots, expected, sizeof slots) == 0);
    selected->state.cnt = wanted->state.cnt = phase;
    selected->state.clr = wanted->state.clr = 0;
    if (provider_edits) {
        selected->gate.parameter = wanted->gate.parameter = 0;
        selected->placed.x = wanted->placed.x = -901;
    }
}

static void unexpected_phase(Actor33A *actor)
{
    (void)actor;
    assert(0 && "wrong actor phase selected");
}

static void run_case(s16 parameter, u16 initial_phase, u16 timer,
                     unsigned actor_index, int use_selector, int edit_after_call)
{
    u8 *bytes = (u8 *)slots;
    for (size_t i = 0; i < sizeof slots; ++i) {
        bytes[i] = (u8)(i * 17u + actor_index);
    }
    Actor33A *actor = &slots[actor_index];
    actor->state.pad0[0] = 0x3a;
    actor->state.pad0[1] = 3;
    actor->state.cnt = initial_phase;
    actor->state.clr = timer;
    actor->gate.parameter = parameter;
    memcpy(expected, slots, sizeof slots);
    index_under_test = actor_index;
    provider_edits = edit_after_call;
    calls = 0;
    if (use_selector) {
        assert(initial_phase == 1);
        func_801814B0(actor);
    } else {
        func_80181550(actor);
    }
    assert(calls == (unsigned)(parameter != 0));
    assert(actor->state.cnt == (parameter ? 6 : initial_phase));
    assert(actor->state.clr == (parameter ? 0 : timer));
    assert(memcmp(slots, expected, sizeof slots) == 0);
}

int main(void)
{
    const s16 parameters[] = {-32768, -256, -1, 0, 1, 255, 256, 32767};
    const u16 timers[] = {0, 1, 0x80ff, 0xffff};
    for (unsigned i = 0; i < 8; ++i) D_80186344[i] = unexpected_phase;
    D_80186344[1] = func_80181550;
    for (size_t p = 0; p < sizeof parameters / sizeof parameters[0]; ++p) {
        for (size_t t = 0; t < sizeof timers / sizeof timers[0]; ++t) {
            for (unsigned slot = 1; slot <= 2; ++slot) {
                for (int edit = 0; edit <= 1; ++edit) {
                    run_case(parameters[p], 1, timers[t], slot, 1, edit);
                    run_case(parameters[p], 0xff01, timers[t], slot, 0, edit);
                }
            }
        }
    }
    return 0;
}

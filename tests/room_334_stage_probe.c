#include "actor_334.h"
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(Actor334) == 268, "room actor stride");
_Static_assert(offsetof(Actor334State, cnt) == 2, "phase offset");
_Static_assert(offsetof(Actor334State, clr) == 0x34, "step offset");
_Static_assert(offsetof(Actor334, counter) == 0xfc, "setup counter");

Actor334Phase D_80186324[6];
u16 D_80186320;

typedef struct {
    unsigned char before[12];
    Actor334 actor;
    unsigned char after[14];
} Slot;

enum Behavior { RETURN, CHANGE, HOLD };
static Slot slots[2], want[2];
static Actor334Phase want_table[6];
static u16 want_room;
static unsigned owner, visits, provider_calls, chosen;
static enum Behavior selected_behavior;
static jmp_buf boundary;

static void compare(void)
{
    assert(!memcmp(slots, want, sizeof slots));
    assert(D_80186320 == want_room);
    for (unsigned i = 0; i < 6; ++i) assert(D_80186324[i] == want_table[i]);
}

static void visit(Actor334 *a, unsigned index)
{
    assert(a == &slots[owner].actor);
    assert(!visits++ && !provider_calls && index == chosen);
    compare();
    if (selected_behavior == HOLD) longjmp(boundary, 1);
    if (selected_behavior == CHANGE) {
        unsigned phase = a->state.cnt;
        a->state.cnt = want[owner].actor.state.cnt = (phase + 1u) % 6u;
        a->counter = want[owner].actor.counter = 0x42c1;
        D_80186320 = want_room = 0xad19;
        D_80186324[a->state.cnt] = want_table[a->state.cnt] = func_80180FF8;
    }
}

static void p0(Actor334 *a) { visit(a, 0); }
static void p1(Actor334 *a) { visit(a, 1); }
static void p2(Actor334 *a) { visit(a, 2); }
static void p3(Actor334 *a) { visit(a, 3); }
static void p4(Actor334 *a) { visit(a, 4); }
static void p5(Actor334 *a) { visit(a, 5); }
static Actor334Phase const probes[6] = {p0, p1, p2, p3, p4, p5};

void func_8012AD50(Actor334State *state)
{
    assert(state == &slots[owner].actor.state && provider_calls++ == 0 && !visits);
    want_room = 1;
    want[owner].actor.counter = 0;
    compare();
    if (selected_behavior == HOLD) longjmp(boundary, 2);
    state->clr = want[owner].actor.state.clr = 0;
    state->cnt = want[owner].actor.state.cnt = (u16)(state->cnt + 1u);
    if (selected_behavior == CHANGE) {
        slots[owner].actor.counter = want[owner].actor.counter = 0xb893;
        D_80186320 = want_room = 0x27c3;
        D_80186324[3] = want_table[3] = probes[1];
        state->pad4[43] = want[owner].actor.state.pad4[43] = 0xd5;
    }
}

static void prepare(unsigned index, unsigned phase, unsigned shift, u16 type)
{
    unsigned char *all = (unsigned char *)slots;
    for (size_t i = 0; i < sizeof slots; ++i)
        all[i] = (unsigned char)(0x97u + i * 13u + phase);
    owner = index;
    slots[owner].actor.state.pad0[0] = (u8)type;
    slots[owner].actor.state.pad0[1] = (u8)(type >> 8);
    slots[owner].actor.state.cnt = (u16)phase;
    slots[owner].actor.state.clr = (u16)(0x1358u + shift);
    slots[owner].actor.counter = (u16)(0x2440u + phase);
    D_80186320 = want_room = (u16)(0x9000u + shift);
    for (unsigned i = 0; i < 6; ++i)
        D_80186324[i] = want_table[i] = probes[(i + shift) % 6u];
    memcpy(want, slots, sizeof slots);
    visits = provider_calls = 0;
}

static void initialize(unsigned direct)
{
    if (setjmp(boundary) == 0) {
        if (direct) func_80180FF8(&slots[owner].actor);
        else func_80180FBC(&slots[owner].actor);
        assert(selected_behavior != HOLD && provider_calls == 1);
    } else {
        assert(selected_behavior == HOLD && provider_calls == 1);
    }
    assert(!visits);
    compare();
}

static void select_phase(unsigned phase, unsigned shift, enum Behavior behavior)
{
    prepare((phase + shift) & 1u, phase, shift, shift ? 0xa703u : 0x334u);
    chosen = (phase + shift) % 6u;
    selected_behavior = behavior;
    if (setjmp(boundary) == 0) {
        func_80180FBC(&slots[owner].actor);
        assert(behavior != HOLD);
    } else {
        assert(behavior == HOLD);
    }
    assert(visits == 1 && provider_calls == 0);
    compare();
    if (behavior == CHANGE) {
        visits = 0;
        selected_behavior = RETURN;
        initialize(0);
    }
}

int main(void)
{
    for (unsigned phase = 0; phase < 6; ++phase) {
        for (unsigned shift = 0; shift < 6; ++shift)
            for (enum Behavior b = RETURN; b <= HOLD; ++b)
                select_phase(phase, shift, b);
        for (enum Behavior b = RETURN; b <= HOLD; ++b) {
            prepare(phase & 1u, phase, 0, 0x334);
            D_80186324[phase] = want_table[phase] = func_80180FF8;
            selected_behavior = b;
            initialize(0);
        }
    }
    for (unsigned phase = 0; phase < 6; ++phase) {
        prepare(phase & 1u, phase, 0, 0x334);
        for (unsigned i = 0; i < 6; ++i)
            D_80186324[i] = want_table[i] = probes[4];
        selected_behavior = RETURN;
        chosen = 4;
        func_80180FBC(&slots[owner].actor);
        assert(visits == 1 && !provider_calls);
        compare();
    }
    const u16 phases[] = {0, 1, 19, 0x7fff, 0x8000, 0xfffe, 0xffff};
    for (unsigned i = 0; i < sizeof phases / sizeof phases[0]; ++i) {
        prepare(i & 1u, phases[i], i, 0x334);
        selected_behavior = RETURN;
        initialize(1);
    }
    return 0;
}

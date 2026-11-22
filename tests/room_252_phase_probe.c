#include "actor_252.h"
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    unsigned char guard_before[16];
    Actor252 actor;
    unsigned char guard_after[16];
} ProbeSlot;

_Static_assert(sizeof(Actor252) == 0x10c, "actor stride");
_Static_assert(offsetof(Actor252, phase) == 2, "unsigned phase halfword");
static ProbeSlot slots[2], expected[2];
static jmp_buf held;
static unsigned count, visited[16], received[16], operation;
static Actor252Phase expected_table[6];

static void record(Actor252 *actor, unsigned handler)
{
    unsigned slot = actor == &slots[0].actor ? 0 : 1;
    assert(actor == &slots[slot].actor && count < sizeof visited / sizeof visited[0]);
    visited[count] = handler;
    received[count++] = slot;
    if (operation == 3) longjmp(held, 1);
    if (operation == 1) {
        /* Installing a new phase and rewriting the old table cell takes effect
         * on a later invocation. The current dispatch has made its selection. */
        unsigned old_phase = actor->phase;
        actor->phase = (old_phase + 2u) % 6u;
        slots[slot].actor.remaining[0xfa] ^= (unsigned char)(0xd0u + handler);
        D_80195174[old_phase] = D_80195174[(old_phase + 3u) % 6u];
    } else if (operation == 2) {
        /* The nested actor's selection observes the caller's table write. */
        operation = 0;
        D_80195174[slots[1].actor.phase] = D_80195174[5];
        func_80182378(&slots[1].actor);
    }
}

static void probe0(Actor252 *actor) { record(actor, 0); }
static void probe1(Actor252 *actor) { record(actor, 1); }
static void probe2(Actor252 *actor) { record(actor, 2); }
static void probe3(Actor252 *actor) { record(actor, 3); }
static void probe4(Actor252 *actor) { record(actor, 4); }
static void probe5(Actor252 *actor) { record(actor, 5); }

Actor252Phase D_80195174[6];
static Actor252Phase const probes[] = {probe0, probe1, probe2, probe3, probe4, probe5};

static void prepare(unsigned phase, unsigned other, unsigned shift)
{
    unsigned char *bytes = (unsigned char *)slots;
    for (size_t i = 0; i < sizeof slots; ++i)
        bytes[i] = (unsigned char)(i * 29u + phase + 7u);
    slots[0].actor.type = slots[1].actor.type = 0x252;
    slots[0].actor.phase = phase;
    slots[1].actor.phase = other;
    for (unsigned i = 0; i < 6; ++i)
        D_80195174[i] = probes[(i + shift) % 6];
    memcpy(expected, slots, sizeof slots);
    memcpy(expected_table, D_80195174, sizeof expected_table);
    count = 0;
    operation = 0;
}

static void unchanged(void)
{
    assert(!memcmp(expected, slots, sizeof slots));
    for (unsigned i = 0; i < 6; ++i)
        assert(expected_table[i] == D_80195174[i]);
}

int main(void)
{
    for (unsigned phase = 0; phase < 6; ++phase) {
        for (unsigned shift = 0; shift < 6; ++shift) {
            unsigned handler = (phase + shift) % 6;
            prepare(phase, 5 - phase, shift);
            func_80182378(&slots[0].actor);
            assert(count == 1 && received[0] == 0 && visited[0] == handler);
            unchanged();

            operation = 1;
            expected[0].actor.phase = (phase + 2) % 6;
            expected[0].actor.remaining[0xfa] ^= (unsigned char)(0xd0u + handler);
            expected_table[phase] = expected_table[(phase + 3) % 6];
            func_80182378(&slots[0].actor);
            assert(count == 2 && received[1] == 0 && visited[1] == handler);
            unchanged();
            operation = 0;
            func_80182378(&slots[0].actor);
            assert(count == 3 && received[2] == 0 && visited[2] == (handler + 2) % 6);
            unchanged();
        }
    }
    prepare(0, 4, 0);
    D_80195174[0] = expected_table[0] = probes[3];
    expected_table[4] = probes[5];
    operation = 2;
    func_80182378(&slots[0].actor);
    assert(count == 2 && visited[0] == 3 && received[0] == 0);
    assert(visited[1] == 5 && received[1] == 1);
    unchanged();

    for (unsigned i = 0; i < 6; ++i)
        D_80195174[i] = expected_table[i] = probes[1];
    count = 0;
    func_80182378(&slots[1].actor);
    assert(count == 1 && received[0] == 1 && visited[0] == 1);
    unchanged();
    prepare(0, 2, 0);
    operation = 3;
    if (setjmp(held) == 0) {
        func_80182378(&slots[1].actor);
        assert(0 && "a held callback cannot complete the outer selector");
    }
    assert(count == 1 && visited[0] == 2 && received[0] == 1);
    unchanged();
    return 0;
}

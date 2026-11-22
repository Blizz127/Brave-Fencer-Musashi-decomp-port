#include "actor_33a.h"
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(Actor33A) == 268, "actor stride");
_Static_assert(offsetof(Actor33A, placed.model_address) == 0x20, "four-byte model address");
_Static_assert(offsetof(Actor33A, placed.x) == 6 && offsetof(Actor33A, placed.y) == 10 &&
               offsetof(Actor33A, placed.z) == 14, "signed position halfwords");

Actor33APhase D_80186344[8];

typedef struct {
    unsigned char before[16];
    Actor33A actor;
    unsigned char after[20];
} Slot;

enum Edge { COMPLETE, ACQUIRE, INITIALIZE, ADVANCE, RELEASE, SELECTED };
static Slot slots[2], expected[2];
static unsigned char models[3][32], expected_models[3][32];
static Actor33APhase expected_table[8];
static Actor33APhase expected_callback;
static unsigned owner, model_slot, edits, calls, selected_calls;
static enum Edge stop;
static u32 acquired;
static jmp_buf boundary;

static Actor33A *actor(void) { return &slots[owner].actor; }
static Actor33A *wanted(void) { return &expected[owner].actor; }

static void compare(void)
{
    assert(!memcmp(slots, expected, sizeof slots));
    assert(!memcmp(models, expected_models, sizeof models));
    for (unsigned i = 0; i < 8; ++i) assert(D_80186344[i] == expected_table[i]);
}

static void reached(enum Edge which, unsigned ordinal)
{
    assert(++calls == ordinal);
    compare();
    if (stop == which) longjmp(boundary, 1);
}

u32 bfm_room_33a_acquire_model(void)
{
    reached(ACQUIRE, 1);
    if (edits) {
        actor()->placed.model_address = wanted()->placed.model_address = 0xace03560u;
        actor()->placed.x = wanted()->placed.x = -2741;
        actor()->placed.tail[183] = wanted()->placed.tail[183] = 0xc1;
    }
    return acquired;
}

void bfm_room_33a_initialize_model(u32 address)
{
    assert(acquired && address == acquired);
    wanted()->placed.model_address = acquired;
    reached(INITIALIZE, 2);
    models[model_slot][15] = expected_models[model_slot][15] = 0x53;
    if (edits) {
        actor()->placed.model_address = wanted()->placed.model_address = 0xed174650u;
        actor()->placed.x = wanted()->placed.x = -3911;
        actor()->placed.y = wanted()->placed.y = 32760;
        actor()->placed.z = wanted()->placed.z = -133;
        actor()->placed.pad8[0] = wanted()->placed.pad8[0] = 0xe3;
        actor()->state.cnt = wanted()->state.cnt = 65535;
    }
}

void func_8012AD50(Actor334State *state)
{
    assert(acquired && state == &actor()->state);
    wanted()->placed.x = 0;
    wanted()->placed.y = -256;
    wanted()->placed.z = 7872;
    reached(ADVANCE, 3);
    state->cnt = wanted()->state.cnt = (u16)(wanted()->state.cnt + 1u);
    state->clr = wanted()->state.clr = 0;
    if (edits) {
        actor()->placed.y = wanted()->placed.y = 1977;
        actor()->placed.model_address = wanted()->placed.model_address = 0x8128c7b0u;
        models[model_slot][21] = expected_models[model_slot][21] = 91;
    }
}

void func_8012C098(char *object)
{
    assert(!acquired && object == (char *)actor());
    wanted()->placed.model_address = 0;
    reached(RELEASE, 2);
    if (edits == 1) {
        memset(object, 0, sizeof(Actor33A));
        memset(wanted(), 0, sizeof(Actor33A));
    } else if (edits == 2) {
        actor()->placed.z = wanted()->placed.z = 119;
        actor()->placed.model_address = wanted()->placed.model_address = 0xfe180090u;
    }
}

static void target(Actor33A *which, Actor33APhase invoked)
{
    assert(which == actor() && invoked == expected_callback);
    assert(!selected_calls && !calls);
    ++selected_calls;
    compare();
    if (stop == SELECTED) longjmp(boundary, 1);
    if (edits) {
        unsigned old = which->state.cnt;
        unsigned next = (old + 1u) % 8u;
        which->state.cnt = wanted()->state.cnt = (u16)next;
        D_80186344[next] = expected_table[next] = func_801814EC;
        which->placed.tail[17] = wanted()->placed.tail[17] = 11;
    }
}

static void p0(Actor33A *a) { target(a, p0); }
static void p1(Actor33A *a) { target(a, p1); }
static void p2(Actor33A *a) { target(a, p2); }
static void p3(Actor33A *a) { target(a, p3); }
static void p4(Actor33A *a) { target(a, p4); }
static void p5(Actor33A *a) { target(a, p5); }
static void p6(Actor33A *a) { target(a, p6); }
static void p7(Actor33A *a) { target(a, p7); }
static Actor33APhase const targets[] = {p0, p1, p2, p3, p4, p5, p6, p7};

static void prepare(unsigned phase, unsigned shift)
{
    unsigned char *bytes = (unsigned char *)slots;
    for (size_t i = 0; i < sizeof slots; ++i) bytes[i] = (u8)(i * 27u + phase + shift);
    owner = (phase + shift) & 1u;
    model_slot = (phase + shift) % 3u;
    for (unsigned j = 0; j < 3; ++j)
        for (unsigned i = 0; i < sizeof models[j]; ++i)
            models[j][i] = (u8)(i * 11u + 3u * j + shift);
    memcpy(expected_models, models, sizeof models);
    actor()->state.pad0[0] = 0x3a;
    actor()->state.pad0[1] = 3;
    actor()->state.cnt = (u16)phase;
    actor()->state.clr = 0x9520u;
    actor()->placed.model_address = 0xd3400010u;
    memcpy(expected, slots, sizeof slots);
    for (unsigned i = 0; i < 8; ++i) D_80186344[i] = expected_table[i] = targets[(i + shift) % 8];
    calls = selected_calls = edits = 0;
    acquired = 0x9a3ee120u;
}

static void initialize(int direct)
{
    if (setjmp(boundary) == 0) {
        if (direct) func_801814EC(actor());
        else func_801814B0(actor());
        assert(stop == COMPLETE && calls == (acquired ? 3u : 2u));
    } else {
        assert(stop != COMPLETE);
        assert(calls == (stop == ACQUIRE ? 1u : stop == ADVANCE ? 3u : 2u));
    }
    compare();
}

enum TableLayout { ROTATED, ALIASED, SWAPPED };

static void select_test(unsigned phase, unsigned shift, enum TableLayout layout, unsigned held)
{
    prepare(phase, shift);
    expected_callback = targets[(phase + shift) % 8];
    if (layout == ALIASED) {
        expected_callback = targets[shift];
        for (unsigned i = 0; i < 8; ++i) D_80186344[i] = expected_table[i] = targets[shift];
    } else if (layout == SWAPPED) {
        unsigned neighbor = (phase + 1u) % 8u;
        Actor33APhase initial = D_80186344[phase];
        D_80186344[phase] = D_80186344[neighbor];
        D_80186344[neighbor] = initial;
        expected_callback = targets[(neighbor + shift) % 8u];
        expected_table[phase] = expected_callback;
        expected_table[neighbor] = targets[(phase + shift) % 8u];
    }
    stop = held ? SELECTED : COMPLETE;
    edits = 1;
    if (setjmp(boundary) == 0) {
        func_801814B0(actor());
        assert(stop == COMPLETE);
    } else {
        assert(stop == SELECTED);
    }
    assert(selected_calls == 1 && !calls);
    compare();
    if (!held) {
        /* The selected callback rebound the following phase, including 7 -> 0. */
        stop = COMPLETE;
        initialize(0);
        assert(selected_calls == 1);
    }
}

int main(void)
{
    const u32 addresses[] = {0, 4, 0x10000u, 0x80000000u, 0xfe729a80u, 0xfffffff0u};
    const u16 phases[] = {0, 1, 6, 7, 0x7fff, 0x8000, 0xfffe, 0xffff};
    for (unsigned p = 0; p < 8; ++p)
        for (unsigned s = 0; s < 8; ++s)
            for (enum TableLayout layout = ROTATED; layout <= SWAPPED; ++layout)
                for (unsigned held = 0; held <= 1; ++held) select_test(p, s, layout, held);
    for (unsigned p = 0; p < sizeof phases / sizeof phases[0]; ++p) {
        for (unsigned v = 0; v < sizeof addresses / sizeof addresses[0]; ++v) {
            for (unsigned e = 0; e < 3; ++e) {
                for (enum Edge edge = COMPLETE; edge <= RELEASE; ++edge) {
                    if ((!addresses[v] && (edge == ADVANCE || edge == INITIALIZE)) ||
                        (addresses[v] && edge == RELEASE)) continue;
                    prepare(phases[p], v);
                    edits = e;
                    acquired = addresses[v];
                    stop = edge;
                    if (phases[p] < 8) D_80186344[phases[p]] = expected_table[phases[p]] = func_801814EC;
                    initialize(phases[p] >= 8 || (e & 1u));
                }
            }
        }
    }
    return 0;
}

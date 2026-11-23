/* Standalone host fixture. The defaults dependency and all persistent storage
 * belong to the fixture. No guest memory or runtime is required. */
#include "handle.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LAYOUT(check, name) typedef char layout_##name[(check) ? 1 : -1]
LAYOUT(CHAR_BIT == 8 && sizeof(int) == 4 && sizeof(short) == 2, scalar_sizes);
LAYOUT(sizeof(TutorialHandleSlot) == 0x4c, stride);
LAYOUT(offsetof(TutorialHandleSlot, descriptor) == 0x00, descriptor);
LAYOUT(offsetof(TutorialHandleSlot, state) == 0x04, state);
LAYOUT(offsetof(TutorialHandleSlot, flags) == 0x08, flags);
LAYOUT(offsetof(TutorialHandleSlot, active_state) == 0x1a, active_state);

TutorialHandleSlot D_801269F0[3];
TutorialHandleSlot *D_80127524, *D_80127528;

enum { NO_SLOT = -1, OUTSIDE_POOL = 3 };
static const unsigned int measured_descriptor = 0x8010fd44u;
static TutorialHandleSlot outside;
static unsigned int cases, allocations, opens, initializers, sequences, total_defaults;
static const char *current_case;
static unsigned int variant;

struct Snapshot {
    TutorialHandleSlot pool[3], outside;
    TutorialHandleSlot *active, *previous;
};

struct Hook {
    int slot, mutate, active, previous;
    unsigned int calls;
    struct Snapshot before;
};

static struct Hook hook;

static void check(int ok, const char *detail)
{
    if (!ok) {
        fprintf(stderr, "%s[%u]: %s\n", current_case, variant, detail);
        exit(EXIT_FAILURE);
    }
}

static TutorialHandleSlot *live_slot(int i)
{
    if (i == NO_SLOT) return NULL;
    if (i == OUTSIDE_POOL) return &outside;
    check(i >= 0 && i < 3, "fixture slot index");
    return &D_801269F0[i];
}

static TutorialHandleSlot *saved_slot(struct Snapshot *s, int i)
{
    if (i == OUTSIDE_POOL) return &s->outside;
    check(i >= 0 && i < 3, "fixture snapshot index");
    return &s->pool[i];
}

static struct Snapshot snapshot(void)
{
    struct Snapshot s;
    memcpy(s.pool, D_801269F0, sizeof(s.pool));
    memcpy(&s.outside, &outside, sizeof(outside));
    s.active = D_80127524;
    s.previous = D_80127528;
    return s;
}

static void compare(const struct Snapshot *s, const char *detail)
{
    check(!memcmp(D_801269F0, s->pool, sizeof(s->pool))
          && !memcmp(&outside, &s->outside, sizeof(outside))
          && D_80127524 == s->active && D_80127528 == s->previous, detail);
}

/* This synthetic dependency deliberately changes data that the initializer
 * overwrites, unrelated bytes that it must preserve, and both globals. */
static void patch_defaults(TutorialHandleSlot *target, TutorialHandleSlot *zero)
{
    target->descriptor = 0x421193d5u;
    target->state = -127;
    target->flags = 0x1020432;
    target->active_state = -77;
    target->reserved06 = 0x568e;
    target->reserved0c[4] ^= 0x8a;
    target->reserved1c[31] ^= 0x41;
    zero->reserved1c[23] ^= 0xb2;
}

void func_80137840(TutorialHandleSlot *slot)
{
    check(slot != NULL && slot == live_slot(hook.slot), "unexpected call to defaults");
    check(++hook.calls == 1, "duplicate defaults call");
    ++total_defaults;
    compare(&hook.before, "persistent state changed before opaque defaults");
    if (hook.mutate) {
        patch_defaults(slot, &D_801269F0[0]);
        D_80127524 = live_slot(hook.active);
        D_80127528 = live_slot(hook.previous);
    }
}

/* mask bit i denotes a nonzero descriptor in pool element i. Slot zero is
 * deliberately independent of both allocatable-slot occupancy bits. */
static void seed(unsigned int mask, int active, int previous, int zero_fill)
{
    int i;
    size_t n;
    for (i = 0; i <= OUTSIDE_POOL; ++i) {
        TutorialHandleSlot *s = live_slot(i);
        unsigned char *bytes = (unsigned char *)s;
        for (n = 0; n < sizeof(*s); ++n)
            bytes[n] = zero_fill ? 0 : (unsigned char)(0x31u + 43u*i + 73u*n);
        if (i < 3) s->descriptor = (mask & (1u << i)) ? 0xa2345680u + (unsigned int)i : 0;
    }
    D_80127524 = live_slot(active);
    D_80127528 = live_slot(previous);
    memset(&hook, 0, sizeof(hook));
    hook.slot = NO_SLOT;
}

static void plan(int selected, int mutate, int changed_active, int changed_previous)
{
    hook.slot = selected;
    hook.mutate = mutate;
    hook.active = changed_active;
    hook.previous = changed_previous;
    hook.calls = 0;
    hook.before = snapshot();
}

/* Expected values, including the sign-extended flags and the active-branch
 * decision, are supplied by each case. Compare every byte after the call. */
static struct Snapshot expected_init(unsigned int descriptor, int signed_flags,
                                     int install, int old_active)
{
    struct Snapshot s = hook.before;
    TutorialHandleSlot *chosen = saved_slot(&s, hook.slot);
    if (hook.mutate) {
        patch_defaults(chosen, &s.pool[0]);
        s.active = live_slot(hook.active);
        s.previous = live_slot(hook.previous);
        old_active = hook.active;
    }
    chosen->state = 1;
    chosen->descriptor = descriptor;
    chosen->flags = signed_flags;
    if (install) {
        if (old_active != NO_SLOT) {
            saved_slot(&s, old_active)->active_state = 2;
            s.previous = live_slot(old_active);
        }
        s.active = live_slot(hook.slot);
    }
    return s;
}

static void run_allocator(unsigned int mask, int want)
{
    struct Snapshot before;
    current_case = "pure allocator occupancy";
    variant = mask;
    seed(mask, OUTSIDE_POOL, 0, 0);
    before = snapshot();
    check(func_801399A8() == live_slot(want), "returned wrong first empty slot");
    check(hook.calls == 0, "allocator called defaults");
    compare(&before, "allocator changed storage");
    ++cases;
    ++allocations;
}

static void check_open(int chosen, int old_active, unsigned int descriptor)
{
    struct Snapshot expected;
    TutorialHandleSlot *answer;
    plan(chosen, 0, NO_SLOT, NO_SLOT);
    expected = chosen == NO_SLOT ? hook.before : expected_init(descriptor, 0, 1, old_active);
    answer = func_8013767C(descriptor);
    check(answer == live_slot(chosen), "open returned wrong pointer");
    check(hook.calls == (unsigned int)(chosen != NO_SLOT), "incorrect open defaults call count");
    compare(&expected, "open storage or current/previous differs");
}

static void run_wrapper(unsigned int mask, int want)
{
    int old_active = mask == 0 ? NO_SLOT : OUTSIDE_POOL;
    current_case = mask == 0 ? "measured initially empty wrapper" : "wrapper occupancy";
    variant = mask;
    /* The original passive trace measured index 13 -> descriptor 8010fd44,
     * three zero descriptor words and null current/previous -> slot 1.
     * Whole-slot zero fill is a fixture choice. Other fields were unmeasured. */
    seed(mask, old_active, mask == 0 ? NO_SLOT : 0, mask == 0);
    check_open(want, old_active, measured_descriptor);
    ++cases;
    ++opens;
}

struct InitCase {
    const char *name;
    unsigned int descriptor;
    int target, old_active, previous, input, stored, install;
    int mutate, changed_active, changed_previous;
};

static const struct InitCase direct[] = {
    {"null current retains previous", 0x42u, 1, NO_SLOT, 0, 0, 0, 1, 0, 0, 0},
    {"live current replaces prior", 0xaabbu, 2, 1, 0, 7, 7, 1, 0, 0, 0},
    {"self current still marked", 0x9988u, 1, 1, 2, 0, 0, 1, 0, 0, 0},
    {"negative low half, active enabled", 0x71u, 2, 0, 2, 0x12348003, -32765, 1, 0, 0, 0},
    {"low half is positive despite signed input", 0x81u, 2, 0, 0, -65535, 1, 1, 0, 0, 0},
    {"only high word bit set", 0x7u, 2, OUTSIDE_POOL, 2, 0x20000000, 0, 1, 0, 0, 0},
    {"bit13 only", 0x61u, 1, 2, 0, 0x2000, 8192, 0, 0, 0, 0},
    {"bit13 and sign bit", 0xeeu, 2, 1, 1, 0x0321a007, -24569, 0, 0, 0, 0},
    {"all flags set, self current", 0x89u, 1, 1, 2, -1, -1, 0, 0, 0, 0},
    {"defaults replace current and previous", 0xdead0123u, 1, 0, 2, 0x54, 84, 1, 1, OUTSIDE_POOL, 2},
    {"defaults clear current", 0x30001233u, 1, 2, NO_SLOT, 0x8000, -32768, 1, 1, NO_SLOT, OUTSIDE_POOL},
    {"defaults select self", 0x89ab0077u, 2, NO_SLOT, 0, 0x1fff, 8191, 1, 1, 2, 1},
    {"bit13 preserves changes by defaults", 0x41u, 1, 2, 0, 0x7fff, 32767, 0, 1, 0, OUTSIDE_POOL},
    {"direct zero descriptor", 0, 2, 1, 2, 0, 0, 1, 0, 0, 0}
};

static void run_initializer(const struct InitCase *c, unsigned int i)
{
    struct Snapshot expected;
    current_case = c->name;
    variant = i;
    seed(7, c->old_active, c->previous, 0);
    plan(c->target, c->mutate, c->changed_active, c->changed_previous);
    expected = expected_init(c->descriptor, c->stored, c->install, c->old_active);
    func_801377B4(c->descriptor, c->input, live_slot(c->target));
    check(hook.calls == 1, "initializer skipped defaults");
    compare(&expected, "initializer changed unexpected storage or globals");
    ++cases;
    ++initializers;
}

static void run_sequence(int zero_first)
{
    current_case = zero_first ? "zero descriptor reusable on next open" : "repeated open until exhausted";
    variant = (unsigned int)zero_first;
    seed(1, NO_SLOT, OUTSIDE_POOL, 0);
    if (zero_first) {
        check_open(1, NO_SLOT, 0);
        check_open(1, 1, measured_descriptor);
    } else {
        check_open(1, NO_SLOT, measured_descriptor);
    }
    check_open(2, 1, 0x19a4b72u);
    check_open(NO_SLOT, 2, 0xb0184421u);
    ++cases;
    ++sequences;
}

int main(void)
{
    /* Table enumerated independently of the proposed allocator. */
    static const int wanted[8] = {1, 1, 2, 2, 1, 1, NO_SLOT, NO_SLOT};
    unsigned int i;
    for (i = 0; i < 8; ++i) {
        run_allocator(i, wanted[i]);
        run_wrapper(i, wanted[i]);
    }
    for (i = 0; i < sizeof(direct)/sizeof(direct[0]); ++i)
        run_initializer(&direct[i], i);
    run_sequence(0);
    run_sequence(1);
    printf("PASS cases=%u allocator=%u wrapper=%u initializer=%u sequences=%u defaults_calls=%u\n",
           cases, allocations, opens, initializers, sequences, total_defaults);
    return 0;
}

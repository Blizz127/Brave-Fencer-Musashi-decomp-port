/* Manufactured actors and services for the Twinpeak contact query and continuation. */
#include "contact_state.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(FusionWord) == 4, "guest word");
_Static_assert(sizeof(TwinpeakPollHandle) == 4, "host guest handle");
_Static_assert(sizeof(TwinpeakContactQueueHandle) == 4, "host queue handle");
_Static_assert(offsetof(TwinpeakPollActor, base) == 0, "callback actor");
_Static_assert(offsetof(TwinpeakPollActor, selected_contact) == 0x184, "contact word");
_Static_assert(sizeof(TwinpeakPollActor) == 420, "observed actor prefix");

enum Service {
    PROBE, FIRST, SECOND, CLEAN_A, CLEAN_B, SET_ACTION, IDLE,
    MODE, INDEX, EMIT, EFFECT, ATTACH, REFRESH, ADVANCE
};

struct Fields {
    unsigned char first;
    unsigned char last;
    FusionHalf middle;
};

unsigned char D_80078EC1;
unsigned char D_800B9A17;
FusionHalf D_801270C0;
TwinpeakActionCallback D_80189EAC[65536];

static TwinpeakPollActor actors[2], expected[2];
static struct Fields expected_fields;
static unsigned slot, seen, limit, cases, calls, resolutions;
static const enum Service *plan;
static FusionWord first_result, second_result, actor_word;

static void check_state(void)
{
    assert(memcmp(actors, expected, sizeof actors) == 0);
    assert(D_80078EC1 == expected_fields.first);
    assert(D_800B9A17 == expected_fields.last);
    assert(D_801270C0 == expected_fields.middle);
}

/* Every service can reach the same actor and globals through a retained alias. */
static void mutate(TwinpeakPollActor *actor, enum Service service,
                   unsigned char *first, unsigned char *last, FusionHalf *middle)
{
    unsigned mark = (unsigned)service + 1;
    actor->base.reserved[6 + mark] ^= (unsigned char)(0x20 + mark);
    actor->selected_contact ^= 0x80000000u;
    actor->base.current_action = (FusionHalf)((actor->base.current_action + mark) % 41);
    actor->handle ^= mark << 23;
    *first = (unsigned char)(0x40 + mark);
    *last = (unsigned char)(0x80 + mark);
    *middle = (FusionHalf)(0x3100 + mark);
}

static void service(enum Service name)
{
    assert(seen < limit && plan[seen] == name);
    if (name == MODE)
        expected_fields.first = 0;
    if (name == ADVANCE) {
        expected_fields.middle = 2;
        expected_fields.last = 0;
    }
    check_state();
    mutate(&actors[slot], name, &D_80078EC1, &D_800B9A17, &D_801270C0);
    mutate(&expected[slot], name, &expected_fields.first, &expected_fields.last,
           &expected_fields.middle);
    ++seen;
    ++calls;
}

static void actor_service(Action21Actor *actor, enum Service name)
{
    assert(actor == &actors[slot].base);
    service(name);
}

FusionWord func_8014E934(Action21Actor *a)
{
    actor_service(a, PROBE);
    return ~(first_result | second_result);
}

FusionWord func_8014CC28(Action21Actor *a)
{
    actor_service(a, FIRST);
    return first_result;
}

FusionWord func_8014F3E8(Action21Actor *a)
{
    actor_service(a, SECOND);
    return second_result;
}

static void cleanup_a(Action21Actor *a) { actor_service(a, CLEAN_A); }
static void cleanup_b(Action21Actor *a) { actor_service(a, CLEAN_B); }
static void unexpected_cleanup(Action21Actor *a) { (void)a; assert(0); }

void func_80147078(Action21Actor *a, unsigned int action)
{
    assert(action == 0);
    actor_service(a, SET_ACTION);
}

void func_80159B70(Action21Actor *a) { actor_service(a, IDLE); }

void func_80154150(Action21Actor *a, unsigned int mode)
{
    assert(mode == 32);
    actor_service(a, MODE);
}

void func_80154A74(Action21Actor *a, unsigned int index)
{
    assert(index == 24);
    actor_service(a, INDEX);
}

FusionWord bfm_twinpeak_actor_word(const Action21Actor *a)
{
    assert(a == &actors[slot].base && seen < limit && plan[seen] == EMIT);
    ++resolutions;
    return actor_word;
}

TwinpeakContactQueueHandle func_80146A6C(int kind, FusionWord actor, int x, int y,
                                        FusionHalf z, FusionWord p5, FusionWord p6)
{
    assert(kind == 22 && actor == actor_word);
    assert(x == 0 && y == 0 && z == 0 && p5 == 0 && p6 == 0);
    assert(resolutions == 1);
    service(EMIT);
    return ~actor_word;
}

void func_80147324(int value)
{
    assert(value == 1103);
    service(EFFECT);
}

void func_801553A8(Action21Actor *a) { actor_service(a, ATTACH); }
void func_801553C0(Action21Actor *a) { actor_service(a, REFRESH); }
void func_80146CA0(Action21Actor *a) { actor_service(a, ADVANCE); }

static void start(unsigned seed, const enum Service *sequence, unsigned length,
                  FusionWord contact, FusionHalf action)
{
    unsigned char *bytes = (unsigned char *)actors;
    for (size_t i = 0; i < sizeof actors; ++i)
        bytes[i] = (unsigned char)(seed * 5 + i * 13 + 9);
    slot = seed & 1;
    actors[slot].selected_contact = contact;
    actors[slot].base.current_action = action;
    memcpy(expected, actors, sizeof actors);
    D_80078EC1 = expected_fields.first = (unsigned char)(0xa1 + slot);
    D_800B9A17 = expected_fields.last = (unsigned char)(0x53 + slot);
    D_801270C0 = expected_fields.middle = (FusionHalf)(0x5137 + slot);
    actor_word = 0xa591802du + seed;
    if (actor_word == (FusionWord)(uintptr_t)&actors[slot])
        actor_word ^= 1u;
    plan = sequence;
    limit = length;
    seen = resolutions = 0;
    ++cases;
}

static void finish(unsigned expected_resolutions)
{
    assert(seen == limit && resolutions == expected_resolutions);
    check_state();
}

static void test_query(void)
{
    static const enum Service with_probe[] = { PROBE, FIRST, SECOND };
    static const enum Service without_probe[] = { FIRST, SECOND };
    for (unsigned i = 0; i < 32; ++i) {
        for (unsigned kind = 0; kind < 4; ++kind) {
            FusionWord flag = i == 0 ? kind : (1u << i) ^ (0xa5a50000u | (kind & 1u));
            first_result = (kind & 1u) ? 1u << i : 0u;
            second_result = (kind & 2u) ? (1u << i) | (1u << ((i + 3u) % 32u)) : 0u;
            if (flag & 1u)
                start(i + kind, without_probe, 2, i, 7);
            else
                start(i + kind, with_probe, 3, i, 7);
            assert(func_801612B8(&actors[slot].base, flag) == (first_result | second_result));
            finish(0);
        }
    }
}

static void test_continue(void)
{
    static const enum Service accepted[] = { MODE, INDEX, EMIT, EFFECT, ATTACH, REFRESH, ADVANCE };
    static const enum Service rejected_a[] = { CLEAN_A, SET_ACTION, IDLE };
    static const enum Service rejected_b[] = { CLEAN_B, SET_ACTION, IDLE };
    /* Extra manufactured slots distinguish the unsigned, unmasked index. */
    static const FusionHalf extended_indices[] = { 0x7fff, 0x8000, 0x8001, 0xffff };
    for (size_t i = 0; i < sizeof D_80189EAC / sizeof D_80189EAC[0]; ++i)
        D_80189EAC[i] = unexpected_cleanup;
    for (unsigned i = 0; i < 41 + sizeof extended_indices / sizeof extended_indices[0]; ++i) {
        FusionHalf action = i < 41 ? (FusionHalf)i : extended_indices[i - 41];
        D_80189EAC[action] = (action & 1u) ? cleanup_b : cleanup_a;
        for (unsigned edge = 0; edge < 2; ++edge) {
            FusionWord sign_clear = edge ? 0x7fffffffu - action : action;
            start(action + edge, accepted, 7, sign_clear, (FusionHalf)action);
            func_8015FBE0(&actors[slot].base);
            finish(1);

            FusionWord sign_set = edge ? 0xffffffffu - action : 0x80000000u + action;
            start(action + edge, (action & 1u) ? rejected_b : rejected_a, 3,
                  sign_set, (FusionHalf)action);
            func_8015FBE0(&actors[slot].base);
            finish(0);
        }
        D_80189EAC[action] = unexpected_cleanup;
    }
}

int main(void)
{
    test_query();
    test_continue();
    printf("Twinpeak contact-state fixture passed (%u cases, %u service calls)\n", cases, calls);
    return 0;
}

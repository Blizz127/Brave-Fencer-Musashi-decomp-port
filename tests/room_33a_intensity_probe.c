#include "actor_33a_intensity.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Synthetic records. The two mounted ring choices are disjoint from the actor.
 * The other layouts exercise the host byte copies at controlled overlap offsets. */
#define ACTOR_BASE 24
#define RING_VIEW(name, start) struct { u8 before[ACTOR_BASE + (start)]; struct Ring ring; } name
typedef union {
    struct {
        u32 before[6];
        Actor33A actor[2];
        u32 gap[9];
        Actor33ARing ring[2];
        u32 after[4];
    } normal;
    RING_VIEW(before_words, 0xf4);
    RING_VIEW(second_high,  0xf6);
    RING_VIEW(second_low,   0xf8);
    RING_VIEW(first_high,   0xfa);
    RING_VIEW(first_low,    0xfc);
    RING_VIEW(after_timer,  0xfe);
    u8 bytes[768];
} Arena;
_Static_assert(sizeof(Actor33A) == 268 && sizeof(struct Ring) == 64, "room record widths");
_Static_assert(offsetof(Arena, normal.actor) == ACTOR_BASE &&
               offsetof(Arena, second_high.ring) == ACTOR_BASE + 0xf6 &&
               offsetof(Arena, second_low.ring) == ACTOR_BASE + 0xf8 &&
               offsetof(Arena, first_high.ring) == ACTOR_BASE + 0xfa &&
               offsetof(Arena, first_low.ring) == ACTOR_BASE + 0xfc, "overlap views");
_Static_assert(_Generic(&func_80181844, void (*)(Actor33A *, struct Ring *): 1, default: 0),
               "initialization wrapper ABI");
_Static_assert(_Generic(&func_80181864, s32 (*)(Actor33A *, struct Ring *): 1, default: 0),
               "update ABI");

s16 D_800B99DA;
static Arena arena;
static u8 expected[sizeof(Arena)];
enum Placement { FIRST_RING, SECOND_RING, BEFORE_WORDS, SECOND_HIGH, SECOND_LOW,
                 FIRST_HIGH, FIRST_LOW, AFTER_TIMER, PLACEMENTS };
typedef struct {
    Actor33A *actor;
    struct Ring *ring;
    size_t timer_offset, ring_offset;
} Pair;

static Pair select_pair(enum Placement which)
{
    const unsigned index = which == SECOND_RING;
    Pair pair = {
        &arena.normal.actor[index], &arena.normal.ring[index].shape,
        offsetof(Arena, normal.actor) + index * sizeof(Actor33A) + 0xfcu,
        offsetof(Arena, normal.ring) + index * sizeof(Actor33ARing)
    };
#define PICK(view) do { pair.ring = &arena.view.ring; pair.ring_offset = offsetof(Arena, view.ring); } while (0)
    switch (which) {
    case FIRST_RING: case SECOND_RING: break;
    case BEFORE_WORDS: PICK(before_words); break;
    case SECOND_HIGH: PICK(second_high); break;
    case SECOND_LOW: PICK(second_low); break;
    case FIRST_HIGH: PICK(first_high); break;
    case FIRST_LOW: PICK(first_low); break;
    case AFTER_TIMER: PICK(after_timer); break;
    default: assert(0 && "unknown placement");
    }
#undef PICK
    assert(pair.timer_offset + 2 <= sizeof arena && pair.ring_offset + sizeof(struct Ring) <= sizeof arena);
    return pair;
}

static void write16(void *bytes, size_t offset, u16 v)
{
    memcpy((u8 *)bytes + offset, &v, sizeof v);
}
static void write32(void *bytes, size_t offset, u32 v)
{
    memcpy((u8 *)bytes + offset, &v, sizeof v);
}
static void prepare(Pair pair, u16 timer, s16 parity)
{
    u8 *bytes = (u8 *)&arena;
    for (size_t i = 0; i < sizeof arena; ++i)
        bytes[i] = (u8)(i * 29u + 17u + timer + (u16)parity);
    write16(&arena, pair.timer_offset, timer);
    memcpy(expected, &arena, sizeof arena);
    D_800B99DA = parity;
}

#ifdef ROOM_33A_WRAPPER_TEST
static Pair current;
static s16 at_entry;
static s32 child_result;
static unsigned callback_count;

/* An opaque child can change the same timer, the ring and the parity source. */
s32 func_80181864(Actor33A *actor, struct Ring *ring)
{
    assert(++callback_count == 1 && actor == current.actor && ring == current.ring);
    assert(D_800B99DA == at_entry && memcmp(expected, &arena, sizeof arena) == 0);
    write32(ring, 0, 0x9caf35d2u);
    write32(expected, current.ring_offset, 0x9caf35d2u);
    write16(actor, 0xfc, 0xd657);
    write16(expected, current.timer_offset, 0xd657);
    D_800B99DA = -193;
    return child_result;
}
static void check(enum Placement which, u16 timer, s16 parity, int wrapper)
{
    (void)wrapper;
    current = select_pair(which);
    prepare(current, timer, parity);
    write16(expected, current.timer_offset, 0);
    static const s32 results[] = {0, 1, -1, 0x10000, INT32_MAX, INT32_MIN};
    child_result = results[((unsigned)which + timer + (u16)parity) % 6u];
    at_entry = parity;
    callback_count = 0;
    func_80181844(current.actor, current.ring);
    assert(callback_count == 1 && D_800B99DA == -193);
    assert(memcmp(expected, &arena, sizeof arena) == 0);
}
#else
static u16 read16(const void *bytes, size_t offset)
{
    u16 v;
    memcpy(&v, (const u8 *)bytes + offset, sizeof v);
    return v;
}
static void check(enum Placement which, u16 timer, s16 parity, int wrapper)
{
    Pair pair = select_pair(which);
    prepare(pair, timer, parity);
    if (wrapper) write16(expected, pair.timer_offset, 0);
    u16 raw = read16(expected, pair.timer_offset);
    s32 value = (s32)raw - ((raw & 0x8000u) ? 65536 : 0);
    if ((u16)parity & 1u) value -= 4;
    if (value > 255) value = 255;
    if (value < 0) value = 0;
    write32(expected, pair.ring_offset, (u32)value);
    write32(expected, pair.ring_offset + 4, 0);
    s32 next = (s32)read16(expected, pair.timer_offset) + 4;
    write16(expected, pair.timer_offset, (u16)next);
    if (wrapper)
        func_80181844(pair.actor, pair.ring);
    else
        assert(func_80181864(pair.actor, pair.ring) == next);
    assert(D_800B99DA == parity);
    assert(memcmp(expected, &arena, sizeof arena) == 0);
}
#endif

int main(void)
{
    static const u16 timers[] = {
        0, 1, 2, 3, 4, 5, 7, 8, 127, 128, 249, 250, 251, 252, 253, 254,
        255, 256, 257, 258, 259, 260, 261, 511, 512, 0x7ffe, 0x7fff, 0x8000,
        0x8001, 0x80ff, 0xff00, 0xff7f, 0xfffa, 0xfffb, 0xfffc, 0xfffd, 0xfffe, 0xffff
    };
    static const s16 parities[] = {0, 1, 2, 3, -2, -1, 0x124, 0x125, INT16_MIN, INT16_MAX};
    for (enum Placement p = FIRST_RING; p < PLACEMENTS; ++p)
        for (size_t t = 0; t < sizeof timers / sizeof timers[0]; ++t)
            for (size_t k = 0; k < sizeof parities / sizeof parities[0]; ++k) {
                check(p, timers[t], parities[k], 1);
#ifndef ROOM_33A_WRAPPER_TEST
                check(p, timers[t], parities[k], 0);
#endif
            }
#ifndef ROOM_33A_WRAPPER_TEST
    for (unsigned timer = 0; timer <= UINT16_MAX; ++timer) {
        check(FIRST_RING, (u16)timer, -2, 0);
        check(SECOND_RING, (u16)timer, -1, 0);
    }
#endif
    return 0;
}

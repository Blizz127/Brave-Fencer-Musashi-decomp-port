/* Synthetic actor pool and model callbacks for the two-pass forest collision query. */
#include "forest_actor_contact.h"
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(ForestContactSlot) == 268, "guest slot stride");
_Static_assert(offsetof(ForestContactSlot, record) == 0x20, "record handle");
_Static_assert(offsetof(ForestContactSlot, collision) == 0x58, "model tag");
_Static_assert(offsetof(ForestContactSlot, flags) == 0x5c, "collision flags");

enum { SLOTS = 96, RECORD_BASE = 0x80048000u, SLOT_BASE = 0x801202a0u, BAD = 0x801e0134u };
ForestContactSlot D_801202A0[SLOTS];
u16 D_800B99DA;
u32 D_801150D8;
FloorPoint D_80126720, D_801152A8;
static FloorContactRecord records[SLOTS];
static _Alignas(8) u8 actor[0x200];
static FloorPoint probe, out;
static unsigned visited[2 * SLOTS], calls, lookups;
static u32 last_handle, refused;
static int trapping, resolver_changes_tag;
static jmp_buf trap;

typedef s32 (*ModelAction)(unsigned, s32, FloorPoint *, FloorPoint *);
static ModelAction on_model;

static void put_half(u8 *base, unsigned offset, u16 value)
{
    memcpy(base + offset, &value, sizeof value);
}
static void put_word(u8 *base, unsigned offset, u32 value)
{
    memcpy(base + offset, &value, sizeof value);
}
static u16 half(const u8 *base, unsigned offset)
{
    u16 value;
    memcpy(&value, base + offset, sizeof value);
    return value;
}
static u32 slot_address(unsigned index)
{
    return SLOT_BASE + (u32)sizeof(ForestContactSlot) * index;
}
static void activate(unsigned i, u16 flags, u32 tag)
{
    assert(i < SLOTS);
    D_801202A0[i].id = (u16)(i + 1);
    D_801202A0[i].record = RECORD_BASE + 0x80u * i;
    D_801202A0[i].collision = tag;
    D_801202A0[i].flags = flags;
}

void *bfm_forest_contact_pointer(u32 address, u32 width)
{
    assert(address && width == sizeof(FloorContactRecord) && D_801150D8 == 0);
    ++lookups;
    last_handle = address;
    if (resolver_changes_tag) {
        assert(address == RECORD_BASE + 42u * 0x80u);
        D_801202A0[42].collision = UINT32_C(0x80000243);
    }
    if (address >= RECORD_BASE && address < RECORD_BASE + 0x80u * SLOTS &&
        ((address - RECORD_BASE) & 0x7fu) == 0)
        return &records[(address - RECORD_BASE) / 0x80];
    return NULL;
}
_Noreturn void bfm_forest_model_invalid_handle(u32 address)
{
    assert(trapping && D_801150D8 == 0);
    refused = address;
    longjmp(trap, 1);
}
s32 func_80135A4C(FloorContactRecord *record, s32 tag, FloorPoint *a, FloorPoint *b)
{
    unsigned i;
    assert(D_801150D8 == 0 && calls < 2 * SLOTS);
    for (i = 0; i < SLOTS && record != &records[i]; ++i) {}
    assert(i < SLOTS && lookups == calls + 1);
    visited[calls++] = i;
    return on_model(i, tag, a, b);
}
static s32 miss(unsigned i, s32 tag, FloorPoint *a, FloorPoint *b)
{
    assert((u32)tag == D_801202A0[i].collision && a == &probe && b == &out);
    D_801150D8 = 0x80; /* The next eligible slot must clear this even after a miss. */
    return 0;
}
static void reset(void)
{
    memset(actor, 0, sizeof actor);
    memset(D_801202A0, 0, sizeof D_801202A0);
    probe = (FloorPoint){(u16)-13, 7, 19, -2};
    out = (FloorPoint){31, 32763, (u16)-47, 9};
    D_80126720 = (FloorPoint){(u16)-21, 1, 23, 2};
    D_801152A8 = (FloorPoint){0, 0, 0, 0};
    D_800B99DA = 0;
    D_801150D8 = UINT32_C(0x41435157);
    calls = lookups = 0;
    resolver_changes_tag = 0;
    last_handle = refused = 0;
    on_model = miss;
    put_half(actor, 6, 701);
    put_half(actor, 10, 702);
    put_half(actor, 14, 703);
}
static void coordinates(const u8 *o, u16 x, u16 y, u16 z)
{
    assert(half(o, 6) == x && half(o, 10) == y && half(o, 14) == z);
}

static void filters(void)
{
    reset();
    /* Unknown exclusions are compared as values, without asking the owner. */
    put_word(actor, 0x174, BAD);
    put_word(actor, 0x178, slot_address(7));
    for (unsigned i = 0; i < SLOTS; ++i) activate(i, 0x0800, 7);
    D_801202A0[0].id = 0;
    D_801202A0[3].collision = 0;
    D_801202A0[4].flags = 0x0400;
    D_801202A0[0].record = D_801202A0[3].record = D_801202A0[4].record = BAD;
    D_801202A0[7].record = BAD;
    /* Bit 27 in the word at +5c must not satisfy mask 0x1800. */
    put_word((u8 *)&D_801202A0[5], 0x5c, 0x08000000u);
    D_801202A0[5].record = BAD;
    assert(func_8014EA4C(actor, &probe, &out, INT_MIN) == 0);
    assert(calls == SLOTS - 5 && lookups == calls && visited[calls-1] == 95);
    unsigned index = 0;
    for (unsigned i = 0; i < SLOTS; ++i)
        if (i != 0 && i != 3 && i != 4 && i != 5 && i != 7)
            assert(visited[index++] == i);
    coordinates(actor, 701, 702, 703);
    reset();
    activate(94, 0x1000, 2);
    activate(95, 0x1800, 3);
    D_801202A0[94].record = D_801202A0[95].record = BAD;
    put_word(actor, 0x174, slot_address(94));
    put_word(actor, 0x178, slot_address(95));
    assert(!func_8014EA4C(actor, &probe, &out, 7));
    assert(calls == 0 && lookups == 0 && D_801150D8 == UINT32_C(0x41435157));
}
static s32 partial_hit(unsigned i, s32 tag, FloorPoint *a, FloorPoint *b)
{
    assert(i == 93 && tag == 3 && a == &probe && b == &out);
    b->x = 331; b->y = (u16)-11; b->z = 77;
    return 1;
}
static void invalid_record(u32 handle, int prior_hit)
{
    reset();
    activate(93, 0x1000, 3);
    activate(95, 0x0800, 4);
    D_801202A0[95].record = handle;
    if (prior_hit) on_model = partial_hit;
    trapping = 1;
    if (setjmp(trap) == 0) {
        (void)func_8014EA4C(actor, &probe, &out, 0);
        abort();
    }
    trapping = 0;
    assert(refused == handle && calls == 1 && visited[0] == 93);
    assert(lookups == (handle ? 2u : 1u));
    if (handle) assert(last_handle == handle);
    if (prior_hit) coordinates(actor, 331, 5, 77);
    else coordinates(actor, 701, 702, 703);
}
static int side_result;
static s32 hit(unsigned index, s32 tag, FloorPoint *a, FloorPoint *b)
{
    assert(index == 42 && (u32)tag == D_801202A0[42].collision && a && b);
    D_801150D8 = side_result ? 5u : 4u;
    return -4;
}
static void slopes(void)
{
    static const s16 inclinations[] = {INT16_MIN, -2049, -2048, 0, 2048, 2049};
    for (unsigned k = 0; k < sizeof inclinations / sizeof inclinations[0]; ++k) {
        for (unsigned phase = 0; phase < 4; ++phase) {
            for (unsigned side = 0; side <= 1; ++side) {
                reset();
                D_800B99DA = (u16)(0x8e1fu | (phase << 5));
                D_801152A8.y = (u16)inclinations[k];
                activate(42, 0x1000, UINT32_C(0x80004020));
                side_result = (int)side;
                on_model = hit;
                const int dx = phase < 2 ? 7 : 8;
                const int dz = phase == 2 ? 7 : 8;
                u8 expected[sizeof actor];
                memcpy(expected, actor, sizeof expected);
                assert(func_8014EA4C(actor, &probe, &out, -1) == (side ? 2 : 1));
                const int steep = inclinations[k] == -2049 || inclinations[k] == 2049;
                if (side)
                    coordinates(actor, (u16)(31 + dx), 702, (u16)(-47 - dz));
                else if (steep)
                    coordinates(actor, (u16)(31 + dx), 32779, (u16)(-47 - dz));
                else
                    coordinates(actor, 31, 32779, (u16)-47);
                put_half(expected, 6, half(actor, 6));
                put_half(expected, 10, half(actor, 10));
                put_half(expected, 14, half(actor, 14));
                assert(memcmp(expected, actor, sizeof expected) == 0 && calls == 1);
            }
        }
    }
    reset();
    side_result = 1;
    activate(42, 0x0810, 1);
    on_model = hit;
    assert(func_8014EA4C(actor, &probe, &out, 0) == 2);
    coordinates(actor, 701, 702, 703);
}

static s32 changed_translation(unsigned i, s32 tag, FloorPoint *a, FloorPoint *b)
{
    assert(i == 42 && tag == 3 && D_801202A0[i].collision == UINT32_C(0x80000243));
    assert(a == &probe && b == &out);
    return 1;
}
static s32 change_pool(unsigned index, s32 tag, FloorPoint *a, FloorPoint *b)
{
    assert(a == &probe && b == &out);
    if (index == 1) {
        assert(tag == 3);
        /* The post-contact sign is read from the live record. */
        D_801202A0[index].collision = UINT32_C(0x90000011);
        D_801152A8.y = 2049;
        activate(24, 0x0800, 6);
        put_word(actor, 0x174, slot_address(31));
        D_800B99DA = 0x40; /* The offsets were selected once on entry. */
        return 1;
    }
    assert(index == 24 && tag == 6);
    coordinates(actor, 38, 32779, (u16)-55);
    D_801150D8 = 1;
    D_801202A0[index].flags |= 0x10;
    b->x = 401;
    b->z = 402;
    return 1;
}
static void live_slots(void)
{
    reset();
    activate(42, 0x1800, 3);
    resolver_changes_tag = 1;
    D_801152A8.y = 2050;
    on_model = changed_translation;
    assert(func_8014EA4C(actor, &probe, &out, 0) == 1 && calls == 1);
    coordinates(actor, 38, 32779, (u16)-55);
    reset();
    activate(1, 0x0800, 3);
    activate(31, 0x1000, 4);
    D_801202A0[31].record = BAD;
    on_model = change_pool;
    assert(func_8014EA4C(actor, &probe, &out, 1) == 3);
    assert(calls == 2 && visited[0] == 1 && visited[1] == 24);
    coordinates(actor, 38, 32779, (u16)-55);
}
static void overlap(void)
{
    struct OutputView { u8 prefix[4]; FloorPoint point; u8 tail[sizeof actor]; } end_view = {0};
    struct ProbeView { u8 prefix[2]; FloorPoint point; u8 tail[sizeof actor]; } probe_view = {0};
    _Static_assert(offsetof(struct OutputView, point) == 4, "overlap at actor+4");
    _Static_assert(offsetof(struct ProbeView, point) == 2, "overlap at actor+2");
    u8 *target;
    FloorPoint *view;
    reset();
    activate(42, 0x1000, 3);
    on_model = hit;
    side_result = 0;
    /* Writing actor.x at +6 changes the output's y before the next load. */
    target = (u8 *)&end_view;
    memcpy(target, actor, sizeof actor);
    view = &end_view.point;
    view->x = 29;
    view->y = (u16)-408;
    view->z = (u16)-32;
    assert(func_8014EA4C(target, &probe, view, 0) == 1);
    coordinates(target, 29, 45, (u16)-32);
    reset();
    activate(42, 0x1000, 4);
    on_model = hit;
    side_result = 1;
    /* The first store changes probe.z at actor+6 before deciding Z's sign. */
    target = (u8 *)&probe_view;
    memcpy(target, actor, sizeof actor);
    view = &probe_view.point;
    view->x = 4;
    view->z = (u16)-80;
    D_80126720.x = 4;
    D_80126720.z = 2;
    out.x = 40;
    out.z = 70;
    assert(func_8014EA4C(target, view, &out, 0) == 2);
    coordinates(target, 33, 702, 78);
    reset();
    activate(42, 0x1000, UINT32_C(0x80012345));
    on_model = hit;
    side_result = 0;
    D_80126720 = (FloorPoint){(u16)-4, 22, 7, 88};
    D_801152A8 = (FloorPoint){(u16)-4, (u16)-4040, 8, -999};
    assert(func_8014EA4C(actor, &D_80126720, &D_801152A8, 0) == 1);
    coordinates(actor, (u16)-11, (u16)-4024, 16);
}
static s32 two_passes(unsigned index, s32 tag, FloorPoint *a, FloorPoint *b)
{
    assert(index == 95 && tag == 5);
    /* The caller specifies xyz in its stack points; w is outside this ABI. */
    if (calls == 1) {
        assert(a->x == (u16)-300 && a->y == 32754 && a->z == 124);
        assert(b->x == 701 && b->y == 686 && b->z == 703);
        put_word(actor, 0x44, 0x80000); /* The parent tested its gate only on entry. */
        put_half(actor, 0x88, 50);
        put_half(actor, 0x8a, 15);
        put_half(actor, 0x8c, (u16)-37);
        b->x = 90; b->y = (u16)-490; b->z = 38;
        return 1;
    }
    assert(calls == 2);
    assert(a->x == 50 && a->y == (u16)-1 && a->z == (u16)-37);
    assert(b->x == 90 && b->y == (u16)-490 && b->z == 38);
    D_801150D8 = 1;
    D_80126720.x = 50;
    D_80126720.z = (u16)-55;
    b->x = 70; b->z = 400;
    return -1;
}
static void caller(void)
{
    reset();
    activate(95, 0x0800, 5);
    on_model = two_passes;
    put_half(actor, 0x88, (u16)-300);
    put_half(actor, 0x8a, 32770);
    put_half(actor, 0x8c, 124);
    put_word(actor, 0x44, 0x80000);
    assert(!func_8014E98C(actor) && !lookups);
    put_word(actor, 0x44, 0);
    put_word(actor, 0x1f8, 0x04000000);
    assert(!func_8014E98C(actor) && !lookups);
    put_word(actor, 0x1f8, 0);
    assert(func_8014E98C(actor) == 3 && calls == 2 && lookups == 2);
    coordinates(actor, 63, (u16)-474, 408);
}
int main(void)
{
    filters();
    invalid_record(0, 0);
    invalid_record(BAD, 0);
    invalid_record(BAD, 1);
    slopes();
    live_slots();
    overlap();
    caller();
    return 0;
}

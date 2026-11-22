/* Synthetic tower actor placement, callback, and cleanup checks. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psx_types.h"

extern s32 func_8012C890(void *placement, u8 *slot, u8 *owner);
u8 D_801940F0[1];
u16 D_801270C4[2], D_801274E4[2];
u32 D_8011DB08;

enum {
    SLOT_BYTES = 268, SLOT_COUNT = 96, POOL = 512,
    PLACEMENT = 48, CHILD = POOL + SLOT_COUNT * SLOT_BYTES + 64,
    ARENA_BYTES = CHILD + 256
};
enum {
    NO_CHILD, EXTERNAL_CHILD, REDIRECT_AFTER_X, REDIRECT_AFTER_Y, REDIRECT_AFTER_AIM,
    X_TO_ACTOR_Y, Y_TO_ACTOR_Z
};
enum { PLACEMENT_AT_CHILD = -1000 };
enum { KEEP_AIM, REPLACE_AIM, PASSIVE_SETUP };

static const u32 arena_key = 0x9a420000u;
static const u32 table_key = 0x854b9860u;
static const u32 shape_key = 0xa6330180u;
static u32 handlers[65536];
static u8 *heap, *expected;
static unsigned slot, placement, owner;
static u16 initial_serial, initial_status;
static unsigned setup_calls, clear_calls, clears_needed;
static unsigned clear_offset[2], clear_size[2];

typedef struct {
    int placement_delta; /* PLACEMENT when it is independent of the slot. */
    unsigned slot_index;
    int child_mode;
    u16 actor_kind, child_kind, aim, serial;
    unsigned has_owner, switch_owner, setup_mode, fail;
} Example;

static const Example *example;

static u16 half(const u8 *p)
{
    return (u16)(p[0] | ((u16)p[1] << 8));
}

static u32 word(const u8 *p)
{
    return (u32)half(p) | ((u32)half(p + 2) << 16);
}

static void set_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void set_word(u8 *p, u32 value)
{
    set_half(p, (u16)value);
    set_half(p + 2, (u16)(value >> 16));
}

static u32 encode(unsigned offset)
{
    return arena_key + offset;
}

static unsigned decode(u32 key)
{
    assert(key >= arena_key && key < arena_key + ARENA_BYTES);
    return key - arena_key;
}

u32 bfm_tower_spawn_address(const void *p)
{
    if (!p) return 0;
    if (p == D_801940F0) return shape_key;
    uintptr_t addr = (uintptr_t)p;
    assert(addr >= (uintptr_t)heap && addr - (uintptr_t)heap < ARENA_BYTES);
    return encode((unsigned)(addr - (uintptr_t)heap));
}

void *bfm_tower_spawn_pointer(u32 key)
{
    if (!key) return NULL;
    if (key == table_key) return handlers;
    return heap + decode(key);
}

static void check_memory(const char *phase)
{
    for (unsigned i = 0; i < ARENA_BYTES; ++i) {
        if (heap[i] == expected[i]) continue;
        fprintf(stderr, "%s: slot %u placement %+d child-mode %d kind %u fail %u, "
                "byte at slot%+d: %02x expected %02x\n", phase, example->slot_index,
                example->placement_delta, example->child_mode, example->child_kind,
                example->fail, (int)i - (int)slot, (unsigned)heap[i], (unsigned)expected[i]);
        abort();
    }
}

static u16 next_serial(u16 current)
{
    return current == UINT16_MAX ? 1 : (u16)(current + 1);
}

/* Build the callback-entry snapshot. Every load reads the current placement:
 * several vectors place it inside the destination slot. */
static void prepare_expected(void)
{
    u8 *a = expected + slot, *d = expected + placement;
    set_half(a, half(d + 6));
    set_half(a + 6, half(d));
    set_half(a + 0x88, half(a + 6));
    set_half(a + 10, half(d + 2));
    set_half(a + 0x8a, half(a + 10));
    u16 z = half(d + 4);
    set_half(a + 12, 0);
    set_half(a + 8, 0);
    set_half(a + 4, 0);
    set_half(a + 14, z);
    set_half(a + 0x8c, z);
    set_half(a + 0x70, half(d + 8));
    set_half(a + 0x72, half(d + 10) & 0xf7ff);
    set_half(a + 0xfc, half(d + 14));
    u32 user = word(d + 16);
    set_word(a + 0x78, shape_key);
    set_word(a + 0xdc, user);
    set_half(a + 0x36, initial_serial);
    set_word(a + 0x64, owner ? encode(owner) : 0);
    set_word(a + 0x68, owner ? 0 : encode(placement));
    set_half(d + 10, half(d + 10) | 0x8000);
    set_half(a + 2, 0);
}

static unsigned selected_child(void)
{
    switch (example->child_mode) {
    case NO_CHILD: return 0;
    case EXTERNAL_CHILD: return CHILD;
    case REDIRECT_AFTER_X: return slot + 0x18;
    case REDIRECT_AFTER_Y: return slot + 0x16;
    case X_TO_ACTOR_Y: return slot + 2;
    case Y_TO_ACTOR_Z: return slot + 4;
    case REDIRECT_AFTER_AIM: return slot + 0x0e;
    default: abort();
    }
}

/* A setup service may replace the owner or model, move the actor, edit the
 * placement, and flag the spawn for cleanup before the factory resumes. */
static void setup_edits(u8 *memory)
{
    u8 *a = memory + slot, *d = memory + placement;
    unsigned child = selected_child();
    if (example->setup_mode == PASSIVE_SETUP) return;
    set_half(a + 6, 0xfed0);
    set_half(a + 10, 0x3a21);
    set_half(a + 14, 0xfff3);
    set_half(a + 0x50, 0x8df1);
    set_half(a + 0x52, 0xd083);
    set_half(a + 0x54, 0x000f);
    if (example->switch_owner)
        set_word(a + 0x64, owner ? 0 : encode(POOL));
    if (example->setup_mode == REPLACE_AIM) set_half(d + 12, 0xff83);
    set_half(d + 10, half(d + 10) ^ 0x1014);
    set_word(a + 0x20, child ? encode(child) : 0);
    if (child) set_half(memory + child, example->child_kind);
    if (example->child_mode == REDIRECT_AFTER_X)
        set_half(a + 0x50, (u16)(encode(CHILD) - half(a + 6)));
    if (example->child_mode == REDIRECT_AFTER_Y)
        set_half(a + 0x52, (u16)(encode(CHILD) - half(a + 10)));
    if (example->child_mode == REDIRECT_AFTER_AIM)
        set_half(d + 12, (u16)encode(CHILD));
}

void bfm_tower_spawn_setup(u32 target, void *actor)
{
    assert(actor == heap + slot && setup_calls++ == 0);
    assert(target == handlers[half(expected + slot)]);
    assert(D_8011DB08 == table_key && D_801270C4[0] == next_serial(initial_serial));
    assert(D_801274E4[0] == (initial_status & 0xfffeu));
    check_memory("setup");
    setup_edits(heap);
    setup_edits(expected);

    D_801274E4[0] = (u16)(0x2bc4u | example->fail);
    if (example->fail) {
        unsigned child = selected_child();
        if (child && (example->child_kind == 1 || example->child_kind == 2)) {
            clear_offset[clears_needed] = child;
            clear_size[clears_needed++] = example->child_kind == 1 ? 132 : 56;
        }
        clear_offset[clears_needed] = slot;
        clear_size[clears_needed++] = SLOT_BYTES;
    }
}

void func_80016714(s8 *destination, s32 size)
{
    assert(setup_calls == 1 && clear_calls < clears_needed);
    unsigned offset = clear_offset[clear_calls];
    assert(destination == (s8 *)(heap + offset));
    assert(size == (s32)clear_size[clear_calls++]);
    check_memory("clear");
    memset(heap + offset, 0, (size_t)size);
    memset(expected + offset, 0, (size_t)size);
}

static void finish_expected(void)
{
    u8 *d = expected + placement, *a = expected + slot;
    if (example->fail) {
        set_half(d + 10, half(d + 10) & 0x7fff);
        return;
    }
    u32 key = word(a + 0x20);
    if (!key) return;
    u8 *child = expected + decode(key);
    if (!word(a + 0x64) && half(child) == 1 && half(d + 12) != 0x7fff)
        set_half(child + 0x12, half(d + 12));

    /* A coordinate write can change the child handle used by the next axis. */
    for (unsigned axis = 0; axis < 3; ++axis) {
        child = expected + decode(word(a + 0x20));
        u16 coordinate = half(a + 6 + 4 * axis);
        u16 offset = half(a + 0x50 + 2 * axis);
        set_half(child + 8 + 2 * axis, (u16)(coordinate + offset));
    }
}

static void run_case(const Example *item)
{
    example = item;
    slot = POOL + SLOT_BYTES * item->slot_index;
    if (item->placement_delta == PLACEMENT_AT_CHILD) placement = CHILD;
    else if (item->placement_delta == PLACEMENT) placement = PLACEMENT;
    else placement = (unsigned)((int)slot + item->placement_delta);
    owner = item->has_owner ? POOL + SLOT_BYTES * ((item->slot_index + 1) % SLOT_COUNT) : 0;
    initial_serial = item->serial;
    initial_status = (u16)(0xf8c0u | (item->slot_index & 1));
    setup_calls = clear_calls = clears_needed = 0;
    D_8011DB08 = table_key;
    D_801270C4[0] = initial_serial;
    D_801270C4[1] = 0xbe27;
    D_801274E4[0] = initial_status;
    D_801274E4[1] = 0xaa93;
    for (unsigned i = 0; i < ARENA_BYTES; ++i)
        heap[i] = (u8)(0xa3u + i * 29u + item->slot_index);
    u8 *d = heap + placement;
    set_half(d, 0x7ffa);
    set_half(d + 2, 0xffed);
    set_half(d + 4, 0x8003);
    set_half(d + 6, item->actor_kind);
    set_half(d + 8, 0xc69a);
    set_half(d + 10, (u16)(0x3ae5u | ((item->slot_index & 1) << 15)));
    set_half(d + 12, item->aim);
    set_half(d + 14, 0xad31);
    set_word(d + 16, 0xa97064b3u);
    if (item->setup_mode == PASSIVE_SETUP) {
        unsigned child = selected_child();
        set_word(heap + slot + 0x20, child ? encode(child) : 0);
        if (child) set_half(heap + child, item->child_kind);
    }
    memcpy(expected, heap, ARENA_BYTES);
    prepare_expected();

    u32 result = (u32)func_8012C890(d, heap + slot, owner ? heap + owner : NULL);
    assert(result == (item->fail ? 0 : encode(slot)));
    assert(clear_calls == clears_needed && setup_calls == 1);
    assert(D_801270C4[0] == next_serial(initial_serial) && D_801270C4[1] == 0xbe27);
    assert(D_801274E4[0] == (0x2bc4u | item->fail) && D_801274E4[1] == 0xaa93);
    finish_expected();
    check_memory("return");
}

int main(void)
{
    const u16 actor_kinds[] = {0, 1, 73, 84, 255, 0xffff};
    const u16 child_kinds[] = {0, 1, 2, 0xffff};
    const u16 aims[] = {0, 0x7fff, 0x8000, 0xffff};
    const u16 serials[] = {0, 1, 0x7fff, 0xfffe, 0xffff};
    heap = malloc(ARENA_BYTES);
    expected = malloc(ARENA_BYTES);
    assert(heap != NULL && expected != NULL);
#if UINTPTR_MAX > UINT32_MAX
    assert((uintptr_t)heap > UINT32_MAX);
#endif
    for (unsigned i = 0; i < 65536; ++i)
        handlers[i] = 0x86470000u + i * 4;

    for (unsigned i = 0; i < SLOT_COUNT; ++i) {
        for (unsigned fail = 0; fail <= 1; ++fail) {
            Example item = {
                PLACEMENT, i, i % 5 == 4 ? NO_CHILD : EXTERNAL_CHILD,
                actor_kinds[i % 6], child_kinds[i % 4], aims[(i / 4) % 4], serials[i % 5],
                i % 2, (i / 2) % 2, (i / 16) % 2, fail
            };
            run_case(&item);
        }
    }

    /* Aim uses the final owner and the placement as the callback leaves them. */
    for (unsigned owned = 0; owned <= 1; ++owned) {
        for (unsigned switch_owner = 0; switch_owner <= 1; ++switch_owner) {
            for (unsigned aim = 0; aim < sizeof aims / sizeof aims[0]; ++aim) {
                for (unsigned setup = KEEP_AIM; setup <= REPLACE_AIM; ++setup) {
                    Example item = {PLACEMENT, 41, EXTERNAL_CHILD, 73, 1, aims[aim],
                                    1, owned, switch_owner, setup, 0};
                    run_case(&item);
                }
            }
        }
    }
    for (unsigned fail = 0; fail <= 1; ++fail) {
        Example item = {PLACEMENT, 21, EXTERNAL_CHILD, 1, 1, 0xfff1,
                        0x8000, 0, 0, PASSIVE_SETUP, fail};
        run_case(&item);
    }

    /* Placement reads around actor positions, owner/template fields, copied
     * positions, and the final user word observe earlier initialization writes. */
    const int displacements[] = {-12, 0, 4, 0x60, 0x68, 0x78, 0x84, 0xcc, 0xf0};
    for (unsigned i = 0; i < sizeof displacements / sizeof displacements[0]; ++i) {
        for (unsigned fail = 0; fail <= 1; ++fail) {
            Example item = {displacements[i], 7, EXTERNAL_CHILD,
                            84, 1, 0x8000, 0xffff, 0, 0, 0, fail};
            run_case(&item);
        }
    }
    const Example overlap[] = {
        /* On failure a child clear can overwrite the placement flags. */
        {PLACEMENT_AT_CHILD, 40, EXTERNAL_CHILD, 84, 1, 0x9c30, 42, 0, 0, 0, 1},
        /* Each axis reads the actor when processed, including child writes aliasing it. */
        {PLACEMENT, 0, X_TO_ACTOR_Y, 84, 2, 0x7fff, 9, 1, 0, 0, 0},
        {PLACEMENT, 30, Y_TO_ACTOR_Z, 84, 2, 0x7fff, 9, 1, 0, 0, 0},
        {PLACEMENT, 12, REDIRECT_AFTER_Y, 84, 2, 0x7fff, 9, 1, 0, 0, 0},
        {PLACEMENT, 0, REDIRECT_AFTER_X, 84, 2, 0x7fff, 9, 1, 0, 0, 0},
        {PLACEMENT, 95, REDIRECT_AFTER_AIM, 84, 1, 0x8001, 0xfffe, 0, 0, 0, 0}
    };
    for (unsigned i = 0; i < sizeof overlap / sizeof overlap[0]; ++i)
        run_case(&overlap[i]);
    free(expected);
    free(heap);
    return 0;
}

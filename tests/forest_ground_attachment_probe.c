#include "forest_ground_attachment.h"

#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

enum {
    SOURCE_BYTES = 0x90,
    SOURCE_HANDLE = 0x100,
    TARGET_HANDLE = 0x200
};
#define HANDLE_BASE 0xe7000000u

typedef struct {
    u8 prefix[SOURCE_BYTES];
    ForestGroundAttachment control;
    u8 suffix[SOURCE_BYTES];
    u8 model[8];
} Fixture;

_Static_assert(sizeof(ForestGroundAttachment) == 0x2c, "controller stride");
_Static_assert(offsetof(ForestGroundAttachment, source) == 4, "source handle");
_Static_assert(offsetof(ForestGroundAttachment, target) == 8, "target handle");
_Static_assert(offsetof(ForestGroundAttachment, x) == 0x20, "ground position");

static Fixture scene;
static u8 *source;
static u8 *target;
static int root_available;
static unsigned effect_calls, returned_calls, orientation_calls;
static u32 effect_index, returned_index, rejected;
static u32 unavailable_table;
static const Fixture *expected_at_orientation;
static s32 callback_result;
static jmp_buf refusal;

ForestGroundAttachment *bfm_forest_ground_attachment_root(void)
{
    return root_available ? &scene.control : NULL;
}

void bfm_forest_ground_attachment_effect(u32 table, u32 index, ForestGroundAttachment *record)
{
    assert(table == 0x801896c0u && record == &scene.control);
    if (unavailable_table == table)
        bfm_forest_ground_attachment_missing(table + 4 * index);
    effect_index = index;
    ++effect_calls;
    /* The consumer calls the selected callback once, even if it changes the step. */
    record->step = 0;
}

s32 bfm_forest_ground_attachment_call(u32 table, u32 index, ForestGroundAttachment *record)
{
    assert(table == 0x801896c4u && record == &scene.control);
    if (unavailable_table == table)
        bfm_forest_ground_attachment_missing(table + 4 * index);
    returned_index = index;
    ++returned_calls;
    record->kind ^= 0xffu;
    return callback_result;
}

u8 *bfm_forest_ground_attachment_source(u32 handle)
{
    return handle == HANDLE_BASE + SOURCE_HANDLE ? source : NULL;
}

u8 *bfm_forest_ground_attachment_target(u32 handle)
{
    return handle == HANDLE_BASE + TARGET_HANDLE ? target : NULL;
}

void bfm_forest_ground_attachment_orient(ForestGroundAttachment *record)
{
    assert(record == &scene.control);
    assert(expected_at_orientation && !memcmp(expected_at_orientation, &scene, sizeof scene));
    ++orientation_calls;
}

_Noreturn void bfm_forest_ground_attachment_missing(u32 handle)
{
    rejected = handle;
    longjmp(refusal, 1);
}

static void test_tables(void)
{
    static const s32 results[] = {INT_MIN, -128, -1, 0, 1, 255, INT_MAX};
    root_available = 1;
    for (unsigned step = 0; step <= 255; ++step) {
        scene.control.step = (u8)step;
        effect_calls = 0;
        func_801627E8();
        assert(effect_calls == (step != 0));
        if (step)
            assert(effect_index == step - 1);
    }
    for (unsigned kind = 0; kind <= 255; ++kind) {
        for (size_t i = 0; i < sizeof results / sizeof *results; ++i) {
            scene.control.kind = (u8)kind;
            callback_result = results[i];
            returned_calls = 0;
            assert(func_80162834(&scene.control) == callback_result);
            assert(returned_calls == 1 && returned_index == kind);
            assert(scene.control.kind == (u8)(kind ^ 255u));
        }
    }
}

static void write_half(u8 *base, size_t offset, s16 value)
{
    memcpy(base + offset, &value, sizeof value);
}

static void update_expected(Fixture *image, size_t source_offset, size_t target_offset)
{
    u8 *bytes = (u8 *)image;
    u32 flags;
    s16 grounded;
    memcpy(&flags, bytes + target_offset + 4, sizeof flags);
    memcpy(&grounded, bytes + source_offset + 0x7e, sizeof grounded);
    flags &= 0x7fffffffu;
    if (grounded)
        flags |= 0x80000000u;
    memcpy(bytes + target_offset + 4, &flags, sizeof flags);
    for (size_t axis = 0; axis < 3; ++axis) {
        s16 component;
        s32 extended;
        memcpy(&component, bytes + source_offset + 0x78 + 2 * axis, sizeof component);
        extended = component;
        memcpy(bytes + offsetof(Fixture, control.x) + 4 * axis, &extended, sizeof extended);
    }
}

static void test_ground(size_t source_offset, size_t target_offset, s16 grounded)
{
    u8 *bytes = (u8 *)&scene;
    Fixture expected;
    memset(&scene, 0xa5, sizeof scene);
    source = bytes + source_offset;
    target = bytes + target_offset;
    write_half(source, 0x78, -32768);
    write_half(source, 0x7a, 0x4567);
    write_half(source, 0x7c, -23);
    write_half(source, 0x7e, grounded);
    scene.control.source = HANDLE_BASE + SOURCE_HANDLE;
    scene.control.target = HANDLE_BASE + TARGET_HANDLE;
    expected = scene;
    update_expected(&expected, source_offset, target_offset);
    orientation_calls = 0;
    expected_at_orientation = &expected;
    func_801628F4(&scene.control);
    expected_at_orientation = NULL;
    assert(orientation_calls == 1 && !memcmp(&expected, &scene, sizeof scene));
}

static void test_aliases(void)
{
    static const s16 flags[] = {0, 1, -1, -32768};
    const size_t destination[] = {
        offsetof(Fixture, model),
        0x74,  /* The flag store overwrites source X and Y. */
        0x78,  /* The flag store overwrites source Z and ground flags. */
        offsetof(Fixture, control.x) - 4
    };
    const size_t origins[] = {
        0,
        offsetof(Fixture, control.x) - 0x7a,  /* Writing X changes the next source halfword. */
        offsetof(Fixture, control.y) - 0x7c
    };
    for (size_t i = 0; i < sizeof origins / sizeof *origins; ++i)
        for (size_t j = 0; j < sizeof destination / sizeof *destination; ++j)
            for (size_t k = 0; k < sizeof flags / sizeof *flags; ++k)
                test_ground(origins[i], destination[j], flags[k]);
}

static void test_refusal(u32 source_handle, u32 target_handle, int root_exists, u32 missing)
{
    Fixture before;
    root_available = root_exists;
    scene.control.source = source_handle;
    scene.control.target = target_handle;
    before = scene;
    orientation_calls = effect_calls = 0;
    rejected = ~missing;
    if (!setjmp(refusal)) {
        if (root_exists)
            func_801628F4(&scene.control);
        else
            func_801627E8();
        assert(0 && "unresolved owner was accepted");
    }
    assert(rejected == missing && !orientation_calls && !effect_calls);
    assert(!memcmp(&before, &scene, sizeof scene));
}

static void test_unavailable_table(u32 table, int returns_value)
{
    Fixture before;
    scene.control.step = 3;
    scene.control.kind = 7;
    root_available = 1;
    unavailable_table = table;
    rejected = effect_calls = returned_calls = 0;
    before = scene;
    if (!setjmp(refusal)) {
        if (returns_value)
            (void)func_80162834(&scene.control);
        else
            func_801627E8();
        assert(0 && "callback without an owner was accepted");
    }
    assert(rejected == table + 4u * (returns_value ? 7u : 2u));
    assert(!effect_calls && !returned_calls && !memcmp(&before, &scene, sizeof scene));
    unavailable_table = 0;
}

int main(void)
{
    test_tables();
    test_aliases();
    test_unavailable_table(0x801896c0u, 0);
    test_unavailable_table(0x801896c4u, 1);
    test_refusal(0, 0, 0, 0x8011f750u);
    test_refusal(0, HANDLE_BASE + TARGET_HANDLE, 1, 0);
    test_refusal(0x100u, HANDLE_BASE + TARGET_HANDLE, 1, 0x100u);
    test_refusal(HANDLE_BASE + SOURCE_HANDLE, 0x200u, 1, 0x200u);
    test_refusal(HANDLE_BASE + SOURCE_HANDLE, 0xffffffffu, 1, 0xffffffffu);
    return 0;
}

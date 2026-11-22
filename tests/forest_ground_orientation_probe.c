#include "forest_ground_orientation.h"

#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

#define SOURCE_HANDLE 0xe7000030u
#define TARGET_HANDLE 0xf1000030u
#define NESTED_HANDLE 0xc3000030u
#define REPLACEMENT_HANDLE 0xb9000030u

enum Service { ATAN_Y = 1, MAGNITUDE, ATAN_X, IDENTITY, ROTATE_Y, ROTATE_X, ROTATE_Z, FINAL_Y };

typedef struct {
    ForestGroundAttachment control;
    u8 storage[0xa0];
    u8 separate_target[0x20];
    u8 first_nested[0x14], next_nested[0x14];
} Scene;

static Scene scene;
static u8 *destination;
static int mutate_fields;
static u32 nested_replacement;
static u32 rejected_handle;
static unsigned completed, unavailable;
static jmp_buf decline;

static struct {
    enum Service operation;
    s32 first, second;
} history[8];

static u8 *controller(void)
{
    return (u8 *)&scene.control;
}

static s16 half(const u8 *bytes, size_t offset)
{
    s16 value;
    memcpy(&value, bytes + offset, sizeof value);
    return value;
}

static void put_half(u8 *bytes, size_t offset, s16 value)
{
    memcpy(bytes + offset, &value, sizeof value);
}

static void put_word(u8 *bytes, size_t offset, u32 value)
{
    memcpy(bytes + offset, &value, sizeof value);
}

static void set_case(s16 dx, s16 dy, s16 dz, int overlap)
{
    memset(&scene, 0x45, sizeof scene);
    destination = overlap ? &scene.storage[0x74] : scene.separate_target;
    put_word(controller(), 4, SOURCE_HANDLE);
    put_word(controller(), 8, TARGET_HANDLE);
    put_word(controller(), 0x24, 0);
    put_half(scene.storage, 0x0a, 0);
    put_word(scene.storage, 0x20, NESTED_HANDLE);
    put_word(scene.storage, 0x44, 0);
    put_half(scene.storage, 0x80, dx);
    put_half(scene.storage, 0x82, dy);
    put_half(scene.storage, 0x84, dz);
    put_half(scene.first_nested, 0x12, 2345);
    put_half(scene.next_nested, 0x12, 123);
    mutate_fields = 0;
    nested_replacement = REPLACEMENT_HANDLE;
    rejected_handle = ~0u;
    completed = unavailable = 0;
}

static void visit(enum Service op, s32 first, s32 second)
{
    assert(completed < sizeof history / sizeof *history);
    assert((unsigned)op == completed + 1);
    if ((unsigned)op == unavailable)
        longjmp(decline, 1);
    history[completed].operation = op;
    history[completed].first = first;
    history[completed].second = second;
    ++completed;
}

static void check_event(enum Service op, s32 first, s32 second)
{
    assert(history[op - 1].operation == op);
    assert(history[op - 1].first == first && history[op - 1].second == second);
}

s32 bfm_forest_orientation_angle(s32 first, s32 second)
{
    if (completed == 0) {
        visit(ATAN_Y, first, second);
        if (mutate_fields) {
            /* The source and destination have already been selected. */
            put_word(controller(), 4, 0x101u);
            put_word(controller(), 8, 0x202u);
            put_half(scene.storage, 0x82, 11);
            put_half(scene.storage, 0x84, -13);
        }
        return 0x1234fff9;
    }
    visit(ATAN_X, first, second);
    return 0x45678122;
}

s32 bfm_forest_orientation_magnitude(s32 square)
{
    visit(MAGNITUDE, square, 0);
    if (mutate_fields)
        put_half(scene.storage, 0x80, -311);
    return 0x32108000;
}

static void opaque_matrix(enum Service op, void *matrix, s32 angle)
{
    assert(matrix == controller() + 0x0c);
    visit(op, angle, 0);
    if (!mutate_fields)
        return;
    switch (op) {
    case IDENTITY:
        assert(half(destination, 0x10) == -7 && half(destination, 0x14) == -32478);
        put_half(destination, 0x10, -22);
        put_half(destination, 0x14, -123);
        put_word(scene.storage, 0x20, nested_replacement);
        break;
    case ROTATE_Y:
        put_half(destination, 0x14, 350);
        break;
    case ROTATE_X:
        put_half(scene.next_nested, 0x12, -2345);
        break;
    case FINAL_Y:
        put_word(matrix, 0x18, 2000);  /* Controller height at +0x24. */
        put_half(scene.storage, 0x0a, 0);
        put_word(scene.storage, 0x44, 0x400);
        break;
    default:
        break;
    }
}

u8 *bfm_forest_orientation_bytes(u32 address, u32 size, enum ForestOrientationRole role)
{
    switch (role) {
    case FOREST_ORIENTATION_SOURCE:
        assert(size == 0x86);
        return address == SOURCE_HANDLE ? scene.storage : NULL;
    case FOREST_ORIENTATION_TARGET:
        assert(size == 0x1e);
        return address == TARGET_HANDLE ? destination : NULL;
    case FOREST_ORIENTATION_SUBOBJECT:
        assert(size == 0x14);
        if (address == NESTED_HANDLE)
            return scene.first_nested;
        if (address == REPLACEMENT_HANDLE)
            return scene.next_nested;
        return NULL;
    }
    assert(0 && "unexpected orientation object role");
    return NULL;
}

_Noreturn void bfm_forest_ground_attachment_missing(u32 address)
{
    rejected_handle = address;
    longjmp(decline, 1);
}

void bfm_forest_orientation_identity(u8 *matrix)
{
    opaque_matrix(IDENTITY, matrix, 0);
}

void bfm_forest_orientation_rotate(u32 service, u8 *matrix, s32 angle)
{
    enum Service operation;
    switch (service) {
    case 0x800126c4u:
        operation = completed < ROTATE_Y ? ROTATE_Y : FINAL_Y;
        break;
    case 0x800123f0u:
        operation = ROTATE_X;
        break;
    case 0x80012558u:
        operation = ROTATE_Z;
        break;
    default:
        assert(0 && "unexpected rotation service");
        return;
    }
    opaque_matrix(operation, matrix, angle);
}

static void check_scales(s16 value)
{
    assert(half(destination, 0x18) == value);
    assert(half(destination, 0x1a) == value);
    assert(half(destination, 0x1c) == value);
}

static void test_numerical_inputs(void)
{
    /* The height difference wraps to a signed halfword. Flag 0x400 then
     * divides the narrowed scale by three, including negative values. */
    static const struct {
        s32 height;
        u16 base;
        u32 flags;
        s16 expected;
    } cases[] = {
        {0, 0, 0, 8192}, {768, 0, 0, 0}, {769, 0, 0, -10},
        {2000, 0, 0x400, -4380}, {32767, 0, 0, -13642},
        {32768, 0, 0, 30037}, {INT_MIN, 65535, 0, 8181},
        {INT_MAX, 65535, 0x400, 2730}, {-26, 65530, 0x400, 2801},
        {32770, 0, 0x400, 10005}, {1234, 65530, 0x400, -1678}
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; ++i) {
        set_case(-14, -32768, -32768, 0);
        put_word(controller(), 0x24, (u32)cases[i].height);
        put_half(scene.storage, 0x0a, (s16)cases[i].base);
        put_word(scene.storage, 0x44, cases[i].flags);
        Scene expected = scene;
        put_half(expected.separate_target, 0x10, -7);
        put_half(expected.separate_target, 0x14, -32478);
        for (unsigned at = 0x18; at <= 0x1c; at += 2)
            put_half(expected.separate_target, at, cases[i].expected);

        func_80162968(&scene.control);
        assert(completed == 8 && !memcmp(&scene, &expected, sizeof scene));
        check_event(ATAN_Y, -32768, -32768);
        check_event(MAGNITUDE, INT_MIN, 0);
        check_event(ATAN_X, -14, 32768);
        check_event(IDENTITY, 0, 0);
        check_event(ROTATE_Y, -7, 0);
        check_event(ROTATE_X, -32478, 0);
        check_event(ROTATE_Z, 2345, 0);
        check_event(FINAL_Y, 0x400, 0);
    }
}

static void test_alias_and_callback_writes(void)
{
    set_case(77, -4, 101, 1);
    func_80162968(&scene.control);
    assert(completed == 8);
    check_event(ATAN_Y, 101, -4);
    /* Destination rotation Y occupies the source Z halfword. */
    check_event(MAGNITUDE, 65, 0);
    check_event(ATAN_X, 77, 32768);
    check_scales(8192);

    /* Rotation X changes the low half of the controller height before scaling. */
    set_case(77, -4, 101, 0);
    destination = (u8 *)&scene + 0x10;
    func_80162968(&scene.control);
    assert(completed == 8);
    check_scales(26944);

    set_case(77, -4, 101, 0);
    mutate_fields = 1;
    func_80162968(&scene.control);
    assert(completed == 8);
    check_event(ATAN_Y, 101, -4);
    check_event(MAGNITUDE, 290, 0);
    check_event(ATAN_X, -311, 32768);
    check_event(ROTATE_Y, -22, 0);
    check_event(ROTATE_X, 350, 0);
    check_event(ROTATE_Z, -2345, 0);
    check_scales(-4380);
}

static int run_transaction(int *changed_before_rollback)
{
    Scene before = scene;
    if (!setjmp(decline)) {
        func_80162968(&scene.control);
        return 1;
    }
    *changed_before_rollback = memcmp(&before, &scene, sizeof scene) != 0;
    scene = before;
    return 0;
}

static void test_refusals(void)
{
    for (unsigned call = ATAN_Y; call <= FINAL_Y; ++call) {
        set_case(-11, -23, 41, 0);
        Scene before = scene;
        int changed = 0;
        unavailable = call;
        assert(!run_transaction(&changed));
        assert(completed == call - 1 && !memcmp(&scene, &before, sizeof scene));
        assert(changed == (call > ATAN_Y));
    }

    for (unsigned changed_handle = 0; changed_handle <= 1; ++changed_handle) {
        set_case(-11, -23, 41, 0);
        mutate_fields = 1;
        if (changed_handle)
            nested_replacement = 0xde000030u;
        else
            unavailable = FINAL_Y;
        Scene before = scene;
        int changed = 0;
        assert(!run_transaction(&changed));
        assert(changed && !memcmp(&scene, &before, sizeof scene));
        assert(completed == (changed_handle ? 6u : 7u));
        if (changed_handle)
            assert(rejected_handle == nested_replacement);
    }

    static const struct {
        unsigned role;
        u32 invalid;
        unsigned prior_calls;
    } bad[] = {
        {0, 0, 0}, {0, 0x30, 0}, {1, 0, 0}, {1, SOURCE_HANDLE, 0},
        {2, 0, 6}, {2, 0x30, 6}, {2, TARGET_HANDLE, 6}
    };
    for (size_t i = 0; i < sizeof bad / sizeof *bad; ++i) {
        set_case(77, -4, 101, 0);
        if (bad[i].role == 0) put_word(controller(), 4, bad[i].invalid);
        if (bad[i].role == 1) put_word(controller(), 8, bad[i].invalid);
        if (bad[i].role == 2) put_word(scene.storage, 0x20, bad[i].invalid);
        Scene before = scene;
        int changed = 0;
        assert(!run_transaction(&changed));
        assert(rejected_handle == bad[i].invalid && completed == bad[i].prior_calls);
        assert(changed == (bad[i].role == 2) && !memcmp(&scene, &before, sizeof scene));
    }
}

int main(void)
{
    test_numerical_inputs();
    test_alias_and_callback_writes();
    test_refusals();
    return 0;
}

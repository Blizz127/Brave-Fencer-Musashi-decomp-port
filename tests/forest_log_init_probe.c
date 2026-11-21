/* Synthetic callback witnesses for the SC01/0077 state-zero initializer. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "psx_types.h"

extern void func_80180590(u8 *actor);
s32 D_801D15C4[1], D_801D1CBC[1];
u8 D_8018A8A4[1], D_8018A8B4[1], D_8018A8C4[1], D_80186E48[1];

static _Alignas(4) u8 actor[0x10c], expected[sizeof(actor)];
static _Alignas(4) u8 model[3][0x84], expected_model[3][0x84];
static u8 initial_variant, variant_after_list, variant_after_setup;
static int allocates, callback_mutations, step;
static int active_model;

static u32 guest_address(const void *p)
{
    uintptr_t address = (uintptr_t)p;
    assert(address <= UINT32_MAX);
    return (u32)address;
}

static void put32(u8 *bytes, size_t offset, u32 value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}

static void put16(u8 *bytes, size_t offset, u16 value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}

static u16 get16(const u8 *bytes, size_t offset)
{
    u16 result;
    memcpy(&result, bytes + offset, sizeof(result));
    return result;
}

static void arrays_equal(void)
{
    assert(memcmp(actor, expected, sizeof(actor)) == 0);
    assert(memcmp(model, expected_model, sizeof(model)) == 0);
}

static void replace_model(int replacement)
{
    active_model = replacement;
    put32(actor, 0x20, guest_address(model[replacement]));
    put32(expected, 0x20, guest_address(model[replacement]));
}

void *func_8012C1B8(void)
{
    assert(step++ == 0);
    arrays_equal();
    return allocates ? model[0] : NULL;
}

s32 func_8012CAE4(u8 *object)
{
    assert(!allocates && step++ == 1 && (void *)object == actor);
    put32(expected, 0x20, 0);
    arrays_equal();
    return -7;
}

void func_8001C214(void *object, s32 *list)
{
    assert(allocates && step++ == 1 && object == model[0]);
    assert(list == (initial_variant == 4 ? D_801D1CBC : D_801D15C4));
    put32(expected, 0x20, guest_address(model[0]));
    arrays_equal();
    if (callback_mutations)
        replace_model(1);
    actor[0x70] = expected[0x70] = variant_after_list;
}

void func_8001D0E8(void *object, s16 width, s16 height)
{
    int bridge = variant_after_list == 4;
    u32 config = guest_address(bridge ? D_8018A8B4 : D_8018A8A4);
    assert(step++ == 2 && object == model[active_model]);
    assert(width == (bridge ? 0x7fff : 0x168));
    assert(height == (bridge ? 0x7fff : 0x140));
    put32(expected, 0x58, config | 0x40000000u);
    arrays_equal();
}

s32 func_8012A828(u8 *object, u32 config)
{
    assert(step++ == 3 && object == actor && config == guest_address(D_80186E48));
    put32(expected, 0x78, guest_address(D_8018A8C4));
    expected[0x75] = 4;
    arrays_equal();
    if (callback_mutations) {
        replace_model(2);
        put16(actor, 0x5c, 0x51a5);
        put16(expected, 0x5c, 0x51a5);
        actor[0x72] = expected[0x72] = 0x39;
    }
    actor[0x70] = expected[0x70] = variant_after_setup;
    return -4;
}

void func_8012B2CC(char *object)
{
    u16 flags = get16(expected, 0x5c);
    assert(step++ == 4 && (void *)object == actor);
    put16(expected, 2, 1);
    put16(expected, 0x5c, variant_after_setup == 4 ? (u16)(flags | 0x8000u) : 0xc800u);
    put16(expected_model[active_model], 0x10, 0);
    arrays_equal();
    /* Verify the final callback's write survives the return. */
    actor[0x76] = expected[0x76] = 0x19;
}

static void run(int success, int mutations, u8 initial, u8 after_list, u8 after_setup)
{
    allocates = success;
    callback_mutations = mutations;
    initial_variant = initial;
    variant_after_list = after_list;
    variant_after_setup = after_setup;
    step = active_model = 0;
    memset(actor, 0xa5, sizeof(actor));
    memset(model, 0x4b, sizeof(model));
    put16(actor, 0x5c, 0x1221);
    actor[0x70] = initial;
    memcpy(expected, actor, sizeof(actor));
    memcpy(expected_model, model, sizeof(model));
    func_80180590(actor);
    assert(step == (success ? 5 : 2));
    arrays_equal();
}

int main(void)
{
    run(0, 0, 4, 4, 4); /* Allocation failure writes a null model before cleanup. */
    run(1, 0, 0, 0, 0);
    run(1, 0, 4, 4, 4);
    run(1, 0, 2, 2, 2);
    run(1, 1, 0, 4, 0); /* Initial list, dimensions and final flags branch independently. */
    run(1, 1, 4, 2, 4); /* Both model replacements are visible to subsequent calls. */
    return 0;
}

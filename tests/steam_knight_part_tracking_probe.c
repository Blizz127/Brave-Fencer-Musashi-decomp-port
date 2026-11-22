#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "steam_knight_hit.h"

extern void func_80186424(SteamKnightPart *part);

_Static_assert(offsetof(SteamKnightPart, model_address) == 0x20, "model address");
_Static_assert(offsetof(SteamKnightPart, body_address) == 0x64, "parent address");
_Static_assert(offsetof(SteamKnightPart, field_fc) == 0xfc, "attachment index");
_Static_assert(sizeof(s16) == 2 && sizeof(s32) == 4, "coordinate widths");

#define GLOBAL_BASE 0x80126b00u
enum { WORLD = 0x5c, MODEL_POINTER = 0x78, TRACK_FLAG = 0x74 };
enum { DETACHED_MODEL = 0x149000, X_MODEL = 0x4000, Y_MODEL = 0x7ffc };
enum Scenario { SEPARATE, RETARGET_X, RETARGET_Y, OVERWRITE_COORDINATES, DISABLED };

typedef struct {
    u8 globals[256];
    u8 model[96];
} Memory;

static Memory guest, expected;
static SteamKnightPart part, expected_part;
static enum Scenario scenario;
static unsigned calls;
static u32 model_address;
static const s32 original_world[3] = { -81777, 1283015, -61359591 };
static const s32 local_result[3] = { 289071, -14119, 766925 };
static s32 world_result[3];
static s16 high[3];

static void set_word(u8 *dst, u32 value)
{
    memcpy(dst, &value, sizeof value);
}

/* The synthetic model can share guest bytes with the globals. */
void *bfm_tower_spawn_pointer(u32 address)
{
    assert(scenario != DISABLED);
    if (address >= GLOBAL_BASE && address - GLOBAL_BASE < sizeof guest.globals)
        return guest.globals + (address - GLOBAL_BASE);
    assert(address == DETACHED_MODEL || address == X_MODEL || address == Y_MODEL);
    return guest.model;
}

void func_8012F0BC(u32 matrix, s32 *world, s32 *local)
{
    assert(calls++ == 0 && scenario != DISABLED);
    assert(matrix == 0x2024u);
    assert(memcmp(world, original_world, sizeof original_world) == 0);
    assert(memcmp(&part, &expected_part, sizeof part) == 0);
    memcpy(local, local_result, sizeof local_result);

    /* Attachment must see the parent/index as they stand after this transform. */
    part.body_address = expected_part.body_address = 0xd34a5980u;
    part.field_fc = expected_part.field_fc = 0xf713;
    ((u8 *)&part)[TRACK_FLAG] = ((u8 *)&expected_part)[TRACK_FLAG] = 0;
}

void bfm_steam_knight_core_attach(SteamKnightPart *actor, u32 parent, int index)
{
    assert(calls++ == (scenario == DISABLED ? 0u : 1u));
    assert(actor == &part);
    assert(memcmp(actor, &expected_part, sizeof part) == 0);
    assert(parent == (scenario == DISABLED ? 0x005ef000u : 0xd34a5980u));
    assert(index == (scenario == DISABLED ? 0x9123 : 0xf713));
    actor->model_address = expected_part.model_address = 0x39000u;
    actor->state = expected_part.state = 0xbb91u;
}

void func_8012F1A4(u32 matrix, s32 *local, s32 *world)
{
    assert(calls++ == 2 && scenario != DISABLED);
    assert(matrix == 0x39034u);
    assert(memcmp(local, local_result, sizeof local_result) == 0);
    assert(memcmp(&part, &expected_part, sizeof part) == 0);
    assert(memcmp(guest.globals + WORLD, original_world, sizeof original_world) == 0);
    memcpy(world, world_result, sizeof world_result);
    set_word(guest.globals + MODEL_POINTER, model_address);
    set_word(expected.globals + MODEL_POINTER, model_address);
}

static void expect_axis(u8 *model, size_t axis, s16 position)
{
    s32 translation = position;
    memcpy(model + 8 + 2 * axis, &position, sizeof position);
    memcpy(model + 0x48 + 4 * axis, &translation, sizeof translation);
}

static void check(enum Scenario which)
{
    scenario = which;
    calls = 0;
    model_address = DETACHED_MODEL;
    memset(&guest, 0xa6, sizeof guest);
    memset(&part, 0x87, sizeof part);
    ((u8 *)&part)[TRACK_FLAG] = which == DISABLED ? 0 : 0x80;
    part.body_address = 0x005ef000u;
    part.model_address = 0x1ff0u;
    part.field_fc = 0x9123;
    memcpy(guest.globals + WORLD, original_world, sizeof original_world);
    memcpy(&expected, &guest, sizeof guest);
    memcpy(&expected_part, &part, sizeof part);

    high[0] = which == RETARGET_X ? X_MODEL : -32768;
    high[1] = which == RETARGET_Y ? Y_MODEL : 32767;
    high[2] = -23;
    const int fraction[3] = { 0xef84, 0x178c, 0x9532 };
    for (int i = 0; i < 3; ++i)
        world_result[i] = high[i] * 65536 + fraction[i];

    if (which == RETARGET_X)
        model_address = GLOBAL_BASE + MODEL_POINTER - 0x48;
    if (which == RETARGET_Y)
        model_address = GLOBAL_BASE + MODEL_POINTER - 0x4c;
    if (which == OVERWRITE_COORDINATES)
        model_address = GLOBAL_BASE + WORLD + 4 - 0x48;

    func_80186424(&part);
    assert(calls == (which == DISABLED ? 1u : 3u));
    assert(memcmp(&part, &expected_part, sizeof part) == 0);

    if (which != DISABLED) {
        memcpy(expected.globals + WORLD, world_result, sizeof world_result);
        u8 *first_model = expected.model;
        if (model_address != DETACHED_MODEL)
            first_model = expected.globals + (model_address - GLOBAL_BASE);
        expect_axis(first_model, 0, high[0]);
        /* X can replace the model pointer before Y, or its translation can
           replace the entire Y coordinate. Y can do the same before Z. */
        if (which == OVERWRITE_COORDINATES) {
            expect_axis(first_model, 1, -1);
            expect_axis(first_model, 2, -1);
        } else {
            expect_axis(which == RETARGET_X ? expected.model : first_model, 1, high[1]);
            expect_axis(expected.model, 2, high[2]);
        }
    }
    assert(memcmp(&guest, &expected, sizeof guest) == 0);
}

int main(void)
{
    check(SEPARATE);
    check(RETARGET_X);
    check(RETARGET_Y);
    check(OVERWRITE_COORDINATES);
    check(DISABLED);
    return 0;
}

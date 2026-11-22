#include "stage_252_light.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(Actor252) == 0x10c && offsetof(Actor252, angle) == 0xfe, "actor layout");
_Static_assert(sizeof(Actor252Light) == 16 && offsetof(Actor252Light, r) == 12, "light packet");

Actor252Light D_800A5EA8, D_801950BC;
Actor252Phase D_80195174[6];

typedef struct {
    uint8_t before[16];
    Actor252 value;
    uint8_t after[16];
} Slot;

enum Stop { COMPLETE, HOLD_ANGLE, HOLD_LIGHT };
static Slot slot, expected_slot;
static Actor252Light expected_work, expected_template;
static int16_t angle_input;
static int32_t supplied;
static enum Stop stop;
static unsigned mutate, calls;
static jmp_buf boundary;

static void compare_state(void)
{
    assert(memcmp(&slot, &expected_slot, sizeof slot) == 0);
    assert(memcmp(&D_800A5EA8, &expected_work, sizeof expected_work) == 0);
    assert(memcmp(&D_801950BC, &expected_template, sizeof expected_template) == 0);
}

/* Use a wide product and explicit signed rewrap before division. */
static int expected_depth(int32_t input)
{
    int64_t full = (int64_t)input * 10;
    int64_t low = (uint32_t)full;
    if (low > INT32_MAX) low -= INT64_C(4294967296);
    return (int)(low / 4096 - 5);
}

s32 func_8004787C(s32 angle)
{
    assert(calls++ == 0 && angle == angle_input);
    expected_work = expected_template;
    compare_state();
    if (stop == HOLD_ANGLE) longjmp(boundary, 1);
    if (mutate) {
        slot.value.angle = expected_slot.value.angle = -31321;
        slot.value.before_angle[53] = expected_slot.value.before_angle[53] = 94;
        D_801950BC.x = expected_template.x = 73005;
        D_800A5EA8.z = expected_work.z = 12007;
        D_800A5EA8.pad = expected_work.pad = 203;
    }
    return supplied;
}

void func_80028620(int index, Actor252Light *light)
{
    assert(calls++ == 1 && index == 2 && light == &D_800A5EA8);
    expected_work.z = expected_depth(supplied);
    expected_slot.value.angle = (uint16_t)((int)expected_slot.value.angle + 113) % 4096u;
    compare_state();
    if (stop == HOLD_LIGHT) longjmp(boundary, 1);
    if (mutate) {
        light->y = expected_work.y = 95713;
        light->g = expected_work.g = 201;
        slot.value.angle = expected_slot.value.angle = -1883;
        D_801950BC.b = expected_template.b = 73;
    }
}

static void run(int16_t angle, int32_t value, enum Stop where, unsigned changes,
                unsigned through_selector)
{
    unsigned char *bytes = (unsigned char *)&slot;
    for (size_t i = 0; i < sizeof slot; ++i)
        bytes[i] = (unsigned char)(23u * i + 0xb4u);
    slot.value.type = 0x252;
    slot.value.phase = 1;
    slot.value.angle = angle;
    expected_slot = slot;
    D_801950BC = (Actor252Light){-24171, 34679, -167889, 103, 239, 38, 149};
    D_800A5EA8 = (Actor252Light){384, -2983, 91873, 23, 58, 71, 194};
    expected_template = D_801950BC;
    expected_work = D_800A5EA8;
    angle_input = angle;
    supplied = value;
    stop = where;
    mutate = changes;
    calls = 0;
    if (setjmp(boundary) == 0) {
        if (through_selector) func_80182378(&slot.value);
        else func_801825A8(&slot.value);
        assert(stop == COMPLETE && calls == 2);
    } else {
        assert((stop == HOLD_ANGLE && calls == 1) || (stop == HOLD_LIGHT && calls == 2));
    }
    compare_state();
}

int main(void)
{
    const int32_t returns[] = {
        0, 1, -1, 409, -409, 410, -410, 4096, -4096, INT32_MAX, INT32_MIN,
        INT32_MIN + 1, 214748365, -214748365, 1073741824, -858993459
    };
    const int16_t angles[] = {0, -1, 1, 3982, 3983, 4095, 4096, INT16_MIN, INT16_MAX};
    D_80195174[1] = func_801825A8;
    for (size_t i = 0; i < sizeof returns / sizeof returns[0]; ++i)
        for (size_t j = 0; j < sizeof angles / sizeof angles[0]; ++j)
            for (unsigned changes = 0; changes <= 1; ++changes)
                for (enum Stop s = COMPLETE; s <= HOLD_LIGHT; ++s)
                    run(angles[j], returns[i], s, changes, (unsigned)(i + j + s) & 1u);
    return 0;
}

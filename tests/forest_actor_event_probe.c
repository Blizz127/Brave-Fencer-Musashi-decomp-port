#include "forest_action_event.h"
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(ForestPlayerState) == 0x240 &&
               offsetof(ForestPlayerState, field_aa) == 0xaa &&
               offsetof(ForestPlayerState, field_16e) == 0x16e &&
               offsetof(ForestPlayerState, field_23c) == 0x23c, "shared player state");
_Static_assert(offsetof(ForestPlayerState, flags) == 0x44, "flag word");
_Static_assert(offsetof(ForestPlayerState, field_ac) == 0xac, "actor request bits");
_Static_assert(sizeof(ForestActionEventRecord) == 20 && sizeof(ForestEventSlot) == 104, "event layout");

ForestEventSlot D_8011F9D0[20];
u8 D_800AE648[32];
static ForestPlayerState actors[2];
static u32 mapped;
static int lookups, fail_on, remap;
static jmp_buf decline;

ForestPlayerState *bfm_forest_event_actor(u32 address)
{
    ++lookups;
    if (!mapped || address != mapped || lookups == fail_on)
        longjmp(decline, 1);
    return &actors[remap && lookups == 2];
}

static void set16(void *dest, u16 value) { memcpy(dest, &value, sizeof(value)); }
static void set32(void *dest, u32 value) { memcpy(dest, &value, sizeof(value)); }

static void trial(u32 flags, u16 bits, u8 game_flags, int vacant)
{
    u8 expected[sizeof(D_8011F9D0)];
    memset(D_8011F9D0, 0xa5, sizeof(D_8011F9D0));
    for (int index = 0; index < 20; ++index) {
        u8 *slot = (u8 *)&D_8011F9D0[index];
        set16(slot, index == vacant || index < 8 ? 0 : (u16)(index + 1));
    }
    memcpy(expected, D_8011F9D0, sizeof(expected));
    memset(D_800AE648, 0x5a, sizeof(D_800AE648));
    D_800AE648[16] = game_flags;
    actors[0].flags = flags;
    actors[0].field_ac = bits;
    int first = (flags & 0x404) == 0 && (bits & 0x80);
    int accepted = first || (!(flags & 0x400) && (game_flags & 1) && (bits & 0x10));
    if (accepted && vacant >= 8) {
        u8 *slot = expected + vacant * sizeof(ForestEventSlot);
        set16(slot, 8);
        set16(slot + 6, 0);
        set16(slot + 10, (u16)-30);
        set16(slot + 14, 0);
        set32(slot + 76, mapped);
        set32(slot + 80, 0);
        set32(slot + 84, 0);
    }
    lookups = fail_on = remap = 0;
    assert(func_801498E0(mapped) == accepted);
    assert(lookups == (first ? 1 : 2));
    assert(memcmp(expected, D_8011F9D0, sizeof(expected)) == 0);
}

static void check_failed_lookup(u32 request, int fault)
{
    u8 before[sizeof(D_8011F9D0)];
    memcpy(before, D_8011F9D0, sizeof(before));
    lookups = remap = 0;
    fail_on = fault;
    if (!setjmp(decline)) {
        (void)func_801498E0(request);
        assert(0);
    }
    assert(lookups == (fail_on ? fail_on : 1));
    assert(memcmp(before, D_8011F9D0, sizeof(before)) == 0);
}

int main(void)
{
    const u32 flags[] = {0, 4, 0x400, 0x404, 0x80000000u, 0x20000004u};
    const u16 bits[] = {0, 0x10, 0x80, 0x90, 0x1000, 0x8080, 0xffff};
    const u8 game_flags[] = {0, 1, 0xfe, 0xff};
    const int slots[] = {-1, 8, 13, 19};
    mapped = 0x80126b58u;
    if (mapped == (u32)(uintptr_t)&actors[0] || mapped == (u32)(uintptr_t)&actors[1])
        mapped += 0x1000;
    for (unsigned f = 0; f < sizeof(flags)/sizeof(flags[0]); ++f)
        for (unsigned b = 0; b < sizeof(bits)/sizeof(bits[0]); ++b)
            for (unsigned g = 0; g < sizeof(game_flags); ++g)
                for (unsigned s = 0; s < sizeof(slots)/sizeof(slots[0]); ++s)
                    trial(flags[f], bits[b], game_flags[g], slots[s]);

    /* A replaced actor backing applies from the second numeric lookup. */
    memset(D_8011F9D0, 0, sizeof(D_8011F9D0));
    actors[0].flags = actors[1].flags = 0;
    actors[0].field_ac = 0;
    actors[1].field_ac = 0x10;
    D_800AE648[16] = 1;
    remap = 1;
    lookups = 0;
    assert(func_801498E0(mapped) == 1 && lookups == 2);
    u32 stored;
    memcpy(&stored, (u8 *)&D_8011F9D0[8] + 76, sizeof(stored));
    assert(stored == mapped);

    check_failed_lookup(mapped, 1);
    check_failed_lookup(mapped, 2);
    check_failed_lookup(mapped + 1, 0);
    check_failed_lookup(0, 0);
    return 0;
}

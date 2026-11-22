#include "steam_knight_hit.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

extern void func_8018550C(SteamKnightPart *part);

u16 D_801E43BC, D_800B99D8, D_801E4BBC;

static SteamKnightPart part, previous_body, current_body;
_Alignas(u32) static u8 table_entry[5];
static u32 slot_address, expected_target;
static u16 effect_mode;
static unsigned count;
static int mutate;
static s32 random_result;
static char calls[16];
static const u32 old_link = 0xa2103000u, current_link = 0x82104000u;

static void note(char event)
{
    assert(count + 1 < sizeof calls);
    calls[count++] = event;
    calls[count] = '\0';
}

void *bfm_tower_spawn_pointer(u32 address)
{
    assert(count == 0 && address == slot_address);
    return table_entry + 1;
}

void bfm_tower_spawn_setup(u32 target, void *actor)
{
    assert(actor == &part && target == expected_target);
    note('S');
    if (mutate)
        part.linked_address = current_link;
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 address)
{
    assert(count == 1 && calls[0] == 'S');
    assert(address == old_link || address == current_link);
    return address == old_link ? &previous_body : &current_body;
}

void func_80186424(SteamKnightPart *actor)
{
    assert(actor == &part);
    note('U');
    if (mutate) {
        current_body.phase_flags = 0x8000u;
        part.linked_address = old_link;
        ((u8 *)actor)[0x74] = 0x59;
        actor->break_count = 1;
        actor->flags = 0x400;
        actor->variant = 17;
    }
}

void func_80132F40(SteamKnightPart *actor)
{
    assert(actor == &part);
    note('V');
    if (mutate) {
        assert(D_801E43BC == (0x8124 | 0x59));
        actor->break_count = 3;
    }
}

void func_80186F60(SteamKnightPart *actor)
{
    assert(actor == &part);
    note('L');
    if (mutate)
        D_800B99D8 = 64;
}

void func_801865D0(SteamKnightPart *actor)
{
    assert(actor == &part);
    note('R');
}

s32 func_8005C49C(void)
{
    note('N');
    if (mutate) {
        part.variant = -17;
        D_801E4BBC = 0xe537;
    }
    return random_result;
}

void func_80189E34(SteamKnightPart *actor, u16 mode)
{
    assert(actor == &part);
    note('E');
    effect_mode = mode;
}

void func_8018A0E4(SteamKnightPart *actor)
{
    assert(actor == &part);
    note('P');
}

static void reset(u16 state, u32 flags)
{
    memset(&part, 0, sizeof part);
    memset(&previous_body, 0, sizeof previous_body);
    memset(&current_body, 0, sizeof current_body);
    part.state = state;
    part.linked_address = current_link;
    previous_body.phase_flags = 0x8000;
    current_body.phase_flags = flags;
    slot_address = 0x801964a8u + 4u * state;
    expected_target = 0xc3020000u + state;
    memcpy(table_entry + 1, &expected_target, sizeof expected_target);
    D_801E43BC = 0x8124;
    D_800B99D8 = 1;
    D_801E4BBC = 0x4fe2;
    count = 0;
    calls[0] = '\0';
    effect_mode = 0;
    mutate = 0;
    random_result = 0;
}

static void changed_by_callbacks(void)
{
    reset(0xffff, 0x14120);
    mutate = 1;
    part.linked_address = old_link;
    random_result = -17;
    func_8018550C(&part);
    assert(strcmp(calls, "SUVLNE") == 0);
    assert(effect_mode == 0xe537 && part.variant == -17);
    assert(part.linked_address == old_link && current_body.phase_flags == 0x8000);
}

static void phase_cases(void)
{
    static const struct {
        u32 phase;
        s16 variant;
        u16 flags, parts, clock;
        u8 status;
        s32 random;
        const char *events;
        u16 status_bits;
    } cases[] = {
        {0x1c120, 17, 0x400, 3, 0, 0xff, 17, "S", 0},
        {0x00120, 17, 0x400, 3, 0, 0x18, 0, "SUVL", 0x18},
        {0x00120, 17, 0,     1, 0, 0x80, 0, "SUR", 0x80},
        {0x00100, 16, 0x400, 2, 0, 0x08, 0, "SU", 0},
        {0x00020, 17, 0x401, 0, 0, 0x08, 0, "SUV", 0},
        {0x00100, 17, 0x400, 1, 0, 0,    0, "SUVR", 0},
        {0x10000, -1, 0, 0, 12, 0, 0x345f, "SUNE", 0},
        {0x10000, -1, 0, 0, 12, 0, -2,     "SUN", 0},
        {0x14000, 0,  0, 0, 60, 0, 1,      "SUN", 0},
        {0x14000, 1,  0, 0, 62, 0, 1,      "SU", 0},
        {0x04000, -32768, 0, 0, 60, 0, 0, "SU", 0},
        {0x04000, 32767,  0, 0, 58, 0, 0, "SU", 0},
        {0x04000,  29,    0, 0, 58, 0, 0, "SUP", 0},
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        reset((u16)(7u * i), cases[i].phase);
        part.variant = cases[i].variant;
        part.flags = cases[i].flags;
        part.break_count = cases[i].parts;
        ((u8 *)&part)[0x74] = cases[i].status;
        D_800B99D8 = cases[i].clock;
        random_result = cases[i].random;
        func_8018550C(&part);
        assert(strcmp(calls, cases[i].events) == 0);
        assert(D_801E43BC == (0x8124 | cases[i].status_bits));
        assert(effect_mode == (strchr(calls, 'E') ? D_801E4BBC : 0));
    }
}

static void clock_wrap(void)
{
    reset(2, 0x4000);
    unsigned phase = 0;
    for (unsigned tick = 0; tick <= UINT16_MAX; ++tick) {
        D_800B99D8 = (u16)tick;
        part.variant = (s16)(phase / 2);
        count = 0;
        func_8018550C(&part);
        assert(strcmp(calls, phase % 2 ? "SU" : "SUP") == 0);
        if (++phase == 60)
            phase = 0;
    }
}

int main(void)
{
    changed_by_callbacks();
    phase_cases();
    clock_wrap();
    return 0;
}

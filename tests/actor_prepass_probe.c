#include "actor_position_updates.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { ACTORS = 96, LAST = ACTORS - 1 };
_Static_assert(sizeof(Obj8014A2E4) == 0x10c, "actor stride");
_Static_assert(offsetof(Obj8014A2E4, hp) == 0xaa, "timer location");

Obj8014A2E4 D_801202A0[ACTORS];
s16 D_801270C0;

static unsigned callback_count, expected_mode;
static s16 expected_time;
static s32 observed_scale;
static int every_actor;
static unsigned refusals;
static Obj8014A2E4 *timer_actor;

static void set_half(Obj8014A2E4 *actor, unsigned offset, u16 value)
{
    u8 *bytes = (u8 *)actor + offset;
    bytes[0] = (u8)value;
    bytes[1] = (u8)(value >> 8);
}

static u16 get_half(const Obj8014A2E4 *actor, unsigned offset)
{
    const u8 *bytes = (const u8 *)actor + offset;
    return (u16)(bytes[0] | (u16)bytes[1] << 8);
}

void func_80178744(u8 *raw, s32 scale)
{
    Obj8014A2E4 *actor = timer_actor;
    assert(raw == (u8 *)actor && actor->hp == expected_time);
    assert(get_half(actor, 0x5e) == expected_mode);
    ++callback_count;
    observed_scale = scale;
    /* The callback owns any changes it makes after the countdown. */
    actor->hp = -331;
    set_half(actor, 0x5e, 0x183b);
}

static int scale_for(int remaining)
{
    if (remaining < 16)
        return 4096 - 144 * remaining;
    if (remaining > 240)
        return 4096 + 144 * (remaining - 256);
    return 1792;
}

static void timer_case(unsigned index, int initial, u16 mode)
{
    Obj8014A2E4 *actor = &D_801202A0[index];
    timer_actor = actor;
    actor->hp = (s16)initial;
    set_half(actor, 0x5e, mode);
    expected_time = (s16)(initial == INT16_MIN ? INT16_MAX : initial - 1);
    expected_mode = initial == 1 && mode == 30 ? 0 : mode;
    callback_count = 0;
    func_8017869C(actor);
    if (initial == 0) {
        assert(callback_count == 0 && actor->hp == 0 && get_half(actor, 0x5e) == mode);
    } else {
        assert(callback_count == 1 && observed_scale == scale_for(expected_time));
        assert(actor->hp == -331 && get_half(actor, 0x5e) == 0x183b);
    }
}

void bfm_actor_update_refused(unsigned reason, u32 key, unsigned offset)
{
    assert(reason == BFM_ACTOR_UPDATE_BAD_ACTOR && key == 0 && offset == 0xaa);
    ++refusals;
}

static void invalid_timers(void)
{
    Obj8014A2E4 other = {0};
    other.hp = 15;
    Obj8014A2E4 *interior = (Obj8014A2E4 *)((u8 *)&D_801202A0[3] + sizeof(u32));
    Obj8014A2E4 *invalid[] = { NULL, interior, D_801202A0 + ACTORS, &other };
    callback_count = 0;
    assert(refusals == 0);
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        func_8017869C(invalid[i]);
        assert(refusals == i + 1 && callback_count == 0);
    }
    assert(other.hp == 15);
}

static void enable_callback(unsigned index, u16 kind, u8 marker)
{
    Obj8014A2E4 *actor = &D_801202A0[index];
    actor->active = kind;
    ((u8 *)actor)[0xc0] = marker;
}

void func_80178840(Obj8014A2E4 *actor)
{
    unsigned index = (unsigned)(actor - D_801202A0);
    if (callback_count == 0) {
        assert(index == 2);
        enable_callback(3, 9, 0xff);
        enable_callback(4, 0, 0x62);
    } else if (callback_count == 1) {
        assert(index == 3);
        enable_callback(0, 8, 0x43);
        enable_callback(LAST, 0xff03, 0x80);
    } else {
        assert(callback_count == 2 && index == LAST);
    }
    ++callback_count;
}

static void active_markers(void)
{
    memset(D_801202A0, 0, sizeof D_801202A0);
    enable_callback(0, 0, 0x80);
    enable_callback(1, 0x4735, 0);
    enable_callback(2, 7, 0x80);
    enable_callback(4, 5, 0x91);
    callback_count = 0;
    func_801787D4();
    assert(callback_count == 3);
}

void func_8012A988(Obj8014A2E4 *actor)
{
    unsigned index = (unsigned)(actor - D_801202A0);
    assert(actor->active == 0);
    if (every_actor) {
        assert(index == callback_count);
    } else if (callback_count == 0) {
        assert(index == 1);
        D_801270C0 = 1;
    } else if (callback_count == 1) {
        assert(index == 2);
        D_801270C0 = -5;
        set_half(&D_801202A0[4], 0x72, 0x2000);
        set_half(&D_801202A0[LAST], 0x72, 0x1800);
    } else {
        assert(callback_count == 2 && index == LAST);
    }
    ++callback_count;
}

static void scene_modes(void)
{
    memset(D_801202A0, 0, sizeof D_801202A0);
    set_half(&D_801202A0[0], 0x72, 0x800);
    set_half(&D_801202A0[1], 0x72, 0x1000);
    set_half(&D_801202A0[4], 0x72, 0x3000);
    D_801270C0 = 2;
    every_actor = 0;
    callback_count = 0;
    func_8012A908();
    assert(callback_count == 3);
    D_801270C0 = 1;
    every_actor = 1;
    callback_count = 0;
    func_8012A908();
    assert(callback_count == ACTORS);
}

int main(void)
{
    if ((uintptr_t)D_801202A0 <= UINT32_MAX)
        return 77;
    for (int initial = INT16_MIN; initial <= INT16_MAX; ++initial)
        timer_case((unsigned)(initial - INT16_MIN) % ACTORS, initial, initial % 2 ? 30 : 0x3a1e);
    timer_case(0, 1, 0x3a1e);
    timer_case(LAST, 0, 30);
    invalid_timers();
    active_markers();
    scene_modes();
    return 0;
}

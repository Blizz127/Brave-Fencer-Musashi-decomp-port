/* Hand-authored actors and service observations for the Steam Knight body action. */
#include "steam_knight_state_three_services.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

void func_80181984(SteamKnightPart *body);

enum { ACTION = 0x34, ANIMATION = 0x94, FRAME = 0x98 };
enum Event { FALLBACK, AIM, SOUND, MOTION, RESET_LINK, START_LINK,
             JOINT, SPAWN, END_LINK, FINISH };
typedef struct {
    enum Event event;
    u16 substate;
    s32 timer;
    u32 flags;
    int argument;
} Observation;

static SteamKnightPart body, link;
static u8 player[8];
_Alignas(4) static u8 packed_link[8];
static u16 stage;
static const u32 link_id = 0xa0001240u, packed_id = 0x80001241u;
static Observation log_entries[6];
static unsigned log_size;
static int mutate_aim;

static void store_half(void *object, unsigned off, u16 value)
{
    memcpy((u8 *)object + off, &value, sizeof value);
}

static u16 load_half(const void *object, unsigned off)
{
    u16 value;
    memcpy(&value, (const u8 *)object + off, sizeof value);
    return value;
}

static void animation(u32 number, u16 frame)
{
    memcpy((u8 *)&body + ANIMATION, &number, sizeof number);
    store_half(&body, FRAME, frame);
}

static void reset(u16 action, s32 timer)
{
    memset(&body, 0xa5, sizeof body);
    memset(&link, 0, sizeof link);
    memset(packed_link, 0, sizeof packed_link);
    memset(player, 0, sizeof player);
    stage = 0;
    body.secondary_address = link_id;
    body.phase_flags = 0;
    body.field_e0 = -1;
    body.timer = timer;
    store_half(&body, ACTION, action);
    animation(0, 0);
    log_size = 0;
    mutate_aim = 0;
}

static void observe(enum Event event, int argument)
{
    assert(log_size < sizeof log_entries / sizeof log_entries[0]);
    log_entries[log_size++] = (Observation){
        event, load_half(&body, ACTION), body.timer, body.phase_flags, argument
    };
}

static void expect(unsigned index, enum Event event, u16 action, s32 timer,
                   u32 flags, int argument)
{
    assert(index < log_size);
    assert(log_entries[index].event == event && log_entries[index].argument == argument);
    assert(log_entries[index].substate == action && log_entries[index].timer == timer);
    assert(log_entries[index].flags == flags);
}

void *bfm_tower_spawn_pointer(u32 address)
{
    if (address == 0x80126b58u) return player;
    if (address == 0x801e4bbcu) return &stage;
    if (address == link_id) return &link;
    assert(address == packed_id);
    return packed_link + 1;
}

void func_80182998(SteamKnightPart *actor)
{
    assert(actor == &body);
    observe(FALLBACK, 0);
}

s32 func_80185D84(SteamKnightPart *actor)
{
    assert(actor == &body);
    observe(AIM, 0);
    if (mutate_aim)
        animation(4, 1);
    return -19;
}

#ifdef BFM_TEST_SOUND_RETURNS_WORD
s32 func_8002D4C8(u16 command, u16 argument)
#else
void func_8002D4C8(u16 command, u16 argument)
#endif
{
    assert(argument == 0);
    observe(SOUND, command);
#ifdef BFM_TEST_SOUND_RETURNS_WORD
    return -41;
#endif
}

void func_80186304(SteamKnightPart *actor, int motion, int mode)
{
    assert(actor == &body && mode == 2);
    observe(MOTION, motion);
}

void func_80186F58(SteamKnightPart *actor)
{
    assert(actor == &body);
    observe(RESET_LINK, 0);
}

void func_801846CC(SteamKnightPart *actor)
{
    assert(actor == &link);
    observe(START_LINK, 0);
}

void func_80186F9C(SteamKnightPart *actor, int joint)
{
    assert(actor == &body);
    observe(JOINT, joint);
}

s32 func_8012C588(s16 kind, SteamKnightSpawnParent *parent)
{
    assert(parent == NULL);
    observe(SPAWN, kind);
    return -9;
}

void func_80184A34(SteamKnightPart *actor)
{
    assert(actor == &link);
    observe(END_LINK, 0);
}

void func_801838A8(SteamKnightPart *actor)
{
    assert(actor == &body);
    observe(FINISH, 0);
}

static void timers(void)
{
    static const struct { u32 start, end; s32 ready; } cases[] = {
        {0, 0, 1}, {1, 0, 1}, {2, 1, 0}, {13, 12, 0},
        {0x7fffffffu, 0x7ffffffeu, 0}, {0x80000000u, 0x7fffffffu, 0},
        {0xffffffffu, 0xfffffffeu, 0}
    };
    unsigned i;
    s32 (*tick)(SteamKnightPart *) = func_8012BEE8;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        SteamKnightPart expected;
        reset(6, 0);
        memcpy(&body.timer, &cases[i].start, sizeof body.timer);
        memcpy(&expected, &body, sizeof expected);
        memcpy(&expected.timer, &cases[i].end, sizeof expected.timer);
        assert(tick(&body) == cases[i].ready);
        assert(memcmp(&expected, &body, sizeof body) == 0 && log_size == 0);
    }
}

static void first_action_ticks(void)
{
    SteamKnightPart expected;
    reset(0, 3);
    body.phase_flags = 0x101;
    memcpy(&expected, &body, sizeof expected);
    func_80181984(&body);
    expected.timer = 2;
    assert(log_size == 0 && !memcmp(&body, &expected, sizeof body));

    animation(4, 1);
    memcpy(&expected, &body, sizeof expected);
    func_80181984(&body);
    expected.timer = 1;
    assert(log_size == 1 && !memcmp(&body, &expected, sizeof body));
    expect(0, SOUND, 0, 2, 0x101, 1272);

    animation(0, 0);
    body.field_e0 = 0;
    mutate_aim = 1;
    log_size = 0;
    func_80181984(&body);
    assert(log_size == 4);
    expect(0, AIM, 0, 1, 0x101, 0);
    expect(1, SOUND, 0, 1, 0x101, 1272);
    expect(2, MOTION, 1, 0, 0x101, 8);
    expect(3, RESET_LINK, 1, 0, 0x100, 0);
    assert(load_half(&body, ACTION) == 1 && body.phase_flags == 0x100);

    log_size = 0;
    animation(3, 0xffff);
    func_80181984(&body);
    assert(!log_size && load_half(&body, ACTION) == 1);
    animation(3, 1);
    func_80181984(&body);
    assert(log_size == 1);
    expect(0, START_LINK, 2, 0, 0x100, 0);

    reset(0, 3);
    animation(4, 0xffff);
    func_80181984(&body);
    assert(!log_size && body.timer == 2);
    animation(3, 1);
    func_80181984(&body);
    assert(!log_size && body.timer == 1);
    body.timer = 0;
    func_80181984(&body);
    assert(log_size == 2);
    expect(0, MOTION, 1, 0, 0, 8);
    expect(1, RESET_LINK, 1, 0, 0, 0);
}

static void linked_action(void)
{
    static const struct { u16 player; int in_range; } positions[] = {
        {0xff40, 0}, {0xff41, 1}, {0xff80, 1}, {0xffcf, 1}, {0xffd0, 0}, {0, 0}
    };
    unsigned i, phase, active_stage, starts_two;
    for (i = 0; i < sizeof positions / sizeof positions[0]; ++i)
        for (phase = 0; phase < 4; ++phase)
            for (active_stage = 0; active_stage < 2; ++active_stage)
                for (starts_two = 0; starts_two < 2; ++starts_two) {
                    u32 flags = (phase & 1 ? 0x2000u : 0u) | (phase & 2 ? 0x80u : 0u);
                    int spawns = !active_stage && !(phase & 2) && positions[i].in_range;
                    s32 duration = phase & 1 ? 32 : 64;
                    SteamKnightPart expected;
                    reset(starts_two ? 2 : 3, 77);
                    body.secondary_address = packed_id;
                    store_half(packed_link + 1, 2, 4);
                    store_half(player, 6, positions[i].player);
                    body.phase_flags = flags;
                    stage = (u16)active_stage;
                    memcpy(&expected, &body, sizeof expected);
                    store_half(&expected, ACTION, 4);
                    expected.timer = duration;
                    if (spawns) expected.phase_flags |= 0x80;
                    func_80181984(&body);
                    assert(!memcmp(&body, &expected, sizeof body));
                    assert(log_size == starts_two * 2 + (unsigned)spawns);
                    if (starts_two) {
                        expect(0, JOINT, 2, 77, flags, 28);
                        expect(1, SOUND, 2, 77, flags, 1264);
                    }
                    if (spawns) expect(starts_two * 2, SPAWN, 4, duration, flags, 94);
                }
    reset(2, 77);
    link.state = 3;
    animation(3, 5);
    func_80181984(&body);
    assert(!log_size && body.timer == 77 && load_half(&body, ACTION) == 2);
    animation(0, 0);
    func_80181984(&body);
    assert(log_size == 2 && load_half(&body, ACTION) == 3 && body.timer == 77);
    log_size = 0;
    func_80181984(&body);
    assert(!log_size && load_half(&body, ACTION) == 3);
}

static void final_actions(void)
{
    SteamKnightPart expected;
    unsigned i;
    static const u16 invalid[] = {6, 7, 0x8000, 0xffff};
    reset(4, 2);
    func_80181984(&body);
    assert(!log_size && body.timer == 1 && load_half(&body, ACTION) == 4);
    func_80181984(&body);
    assert(log_size == 2);
    expect(0, MOTION, 5, 0, 0, 9);
    expect(1, END_LINK, 5, 0, 0, 0);
    log_size = 0;
    animation(0, 1);
    func_80181984(&body);
    assert(!log_size);
    animation(0, 0);
    func_80181984(&body);
    assert(log_size == 1);
    expect(0, FINISH, 5, 0, 0, 0);

    for (i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        reset(invalid[i], -12);
        memcpy(&expected, &body, sizeof expected);
        func_80181984(&body);
        assert(!log_size && !memcmp(&body, &expected, sizeof body));
    }
    for (i = 1; i <= 3; ++i) {
        reset(0, 1);
        animation(4, 1);
        body.phase_flags = i << 17;
        memcpy(&expected, &body, sizeof expected);
        func_80181984(&body);
        assert(log_size == 1 && !memcmp(&body, &expected, sizeof body));
        expect(0, FALLBACK, 0, 1, i << 17, 0);
    }
}

int main(void)
{
    timers();
    first_action_ticks();
    linked_action();
    final_actions();
    return 0;
}

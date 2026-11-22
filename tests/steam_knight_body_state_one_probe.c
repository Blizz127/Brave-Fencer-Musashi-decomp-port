#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "steam_knight_hit.h"

extern void func_801877FC(SteamKnightPart *);

_Static_assert(offsetof(SteamKnightPart, phase_flags) == 0xe8, "phase bits");
_Static_assert(offsetof(SteamKnightPart, break_delay) == 0x100, "signed timer");
_Static_assert(_Generic(((SteamKnightPart *)0)->break_delay, s16: 1, default: 0), "timer type");

typedef struct {
    s32 id;
    s16 frame;
} Animation;

typedef struct {
    int first_result, animation_ready;
    u32 after_first, after_reset;
    s16 service_timer, decremented_timer;
    Animation after_query, after_advance;
} Scenario;

typedef struct {
    u32 before[4];
    SteamKnightPart actor;
    u32 after[4];
} Fixture;

static Fixture live, expected;
static Scenario input;
static const char *pending;

static void service(char name, SteamKnightPart *actor)
{
    assert(*pending == name);
    ++pending;
    assert(actor == &live.actor);
    assert(memcmp(&live, &expected, sizeof live) == 0);
}

static void set_animation(SteamKnightPart *actor, Animation animation)
{
    memcpy((u8 *)actor + 0x94, &animation.id, sizeof animation.id);
    memcpy((u8 *)actor + 0x98, &animation.frame, sizeof animation.frame);
}

int func_8012CE2C(SteamKnightPart *actor)
{
    service('P', actor);
    actor->phase_flags = expected.actor.phase_flags = input.after_first;
    return input.first_result;
}

void func_8012CBCC(SteamKnightPart *actor)
{
    service('R', actor);
    actor->phase_flags = expected.actor.phase_flags = input.after_reset;
}

void func_80182998(SteamKnightPart *actor)
{
    service('E', actor);
    actor->phase_flags = expected.actor.phase_flags = 0;
}

void func_8018655C(SteamKnightPart *actor)
{
    service('T', actor);
    actor->break_delay = expected.actor.break_delay = input.service_timer;
    actor->phase_flags = expected.actor.phase_flags = 0x60000u;
}

int func_8012BEE8(SteamKnightPart *actor)
{
    /* The caller must have applied the signed timer step before this call. */
    expected.actor.break_delay = input.decremented_timer;
    service('Q', actor);
    set_animation(actor, input.after_query);
    set_animation(&expected.actor, input.after_query);
    return input.animation_ready;
}

void func_801838A8(SteamKnightPart *actor)
{
    service('A', actor);
    set_animation(actor, input.after_advance);
    set_animation(&expected.actor, input.after_advance);
}

void func_8002D4C8(u16 command, u16 argument)
{
    service('S', &live.actor);
    assert(command == 1271 && argument == 0);
}

static void run(Scenario scenario, u32 initial_phase, const char *sequence)
{
    memset(&live, 0xad, sizeof live);
    live.actor.phase_flags = initial_phase;
    live.actor.break_delay = 2345;
    set_animation(&live.actor, (Animation){0x419, -7});
    memcpy(&expected, &live, sizeof live);
    input = scenario;
    pending = sequence;
    func_801877FC(&live.actor);
    assert(*pending == '\0' && memcmp(&live, &expected, sizeof live) == 0);
}

static void check_phase_changes(void)
{
    /* P handles the initial predicate, R optionally changes the phase, and
       either phase bit selects E before timer, animation or sound work. */
    run((Scenario){.first_result = -4, .after_first = 0x20000}, 0, "PE");
    run((Scenario){.first_result = 3, .after_first = 0x40000}, 0, "PE");
    run((Scenario){.after_first = 0, .after_reset = 0x60000}, 0, "PRE");
    run((Scenario){.after_first = 0x20000, .after_reset = 0x80001000u,
                   .service_timer = 1, .decremented_timer = 0,
                   .after_query = {4, 1}}, 0x40000, "PRTQS");
}

static void check_timer_and_animation(void)
{
    Scenario s = {.first_result = 1, .after_first = 0x10000,
                  .after_query = {12, 1}, .service_timer = 32767,
                  .decremented_timer = 32766};
    run(s, 0x60000, "PTQS");

    s.service_timer = s.decremented_timer = -32768;
    s.animation_ready = -5;
    s.after_query = (Animation){0x10004, 1};
    s.after_advance = (Animation){12, 1};
    run(s, 0, "PTQAS");

    s.service_timer = s.decremented_timer = 0;
    s.after_query = (Animation){12, 1};
    s.after_advance = (Animation){4, -1};
    run(s, 0, "PTQA");

    s.animation_ready = 0;
    s.service_timer = s.decremented_timer = -1;
    s.after_query = (Animation){0x10004, 1};
    run(s, 0, "PTQ");
    s.after_query = (Animation){0x1000c, 1};
    run(s, 0, "PTQ");
    s.after_query = (Animation){4, 0x101};
    run(s, 0, "PTQ");
    s.after_query = (Animation){12, 0};
    run(s, 0, "PTQ");
}

int main(void)
{
    check_phase_changes();
    check_timer_and_animation();
    return 0;
}

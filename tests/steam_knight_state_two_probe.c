#include "steam_knight_hit.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

extern void func_80182810(SteamKnightPart *);
extern void func_80182918(SteamKnightPart *);
extern void func_801838A8(SteamKnightPart *);
extern void func_8018791C(SteamKnightPart *);

#if defined(__ELF__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TEST_ALIASED_MODE 1
SteamKnightPart overlapping_actor;
_Static_assert(offsetof(SteamKnightPart, phase_flags) == 232, "mode overlay");
__asm__(".globl D_801E4BBC\n.set D_801E4BBC, overlapping_actor + 232");
extern u16 D_801E4BBC;
#else
u16 D_801E4BBC;
#endif

static SteamKnightPart actor;
static SteamKnightPart *active;
static char events[32];
static size_t used;
static int flip_speed, selected_motion, state_timer, trigger, tail_clears_flag;
static s16 decision, next_frame;
static s32 next_motion;
static u32 table_token = 0x13572468u;

static void event(char id)
{
    assert(used + 1 < sizeof(events));
    events[used++] = id;
    events[used] = 0;
}

static void reset(SteamKnightPart *body)
{
    active = body;
    *body = (SteamKnightPart){0};
    D_801E4BBC = 0;
    used = 0;
    events[0] = 0;
    flip_speed = selected_motion = state_timer = trigger = tail_clears_flag = 0;
    decision = next_frame = 0;
    next_motion = 0;
}

static void animation(s32 motion, s16 frame)
{
    memcpy((u8 *)active + 0x94, &motion, sizeof(motion));
    memcpy((u8 *)active + 0x98, &frame, sizeof(frame));
}

void *bfm_tower_spawn_pointer(u32 address)
{
    if (address == 0x801E4BBCu)
        return &D_801E4BBC;
    assert(decision != 0 && address == 0x801962F8u + (u32)(4 * (s32)decision));
    event('L');
    return &table_token;
}

void bfm_tower_spawn_setup(u32 token, void *body)
{
    assert(token == table_token && body == active);
    event('I');
}

s16 func_80186144(SteamKnightPart *body, int joint)
{
    assert(body == active && joint == 5);
    event('J');
    body->phase_flags |= 0x40;
    D_801E4BBC = 1;
    return -123;
}

void func_80186304(SteamKnightPart *body, int motion, int mode)
{
    assert(body == active && mode == 0 && (body->phase_flags & 0x1000));
    event('M');
    selected_motion = motion;
    if (flip_speed)
        body->phase_flags ^= 0x2000;
}

void func_8012AD44(u8 *body, u16 state)
{
    assert(body == (u8 *)active && (active->phase_flags & 1) && state == 2);
    event('S');
    state_timer = active->timer;
    active->state = state;
}

void func_80186F14(SteamKnightPart *body)
{
    assert(body == active && body->state == 2);
    event('F');
    body->phase_flags |= 0x4080;
}

s16 func_80185E80(SteamKnightPart *body)
{
    assert(body == active && !(body->phase_flags & 0x1000));
    event('Q');
    return decision;
}

void func_80182740(SteamKnightPart *body) { (void)body; abort(); }
void func_801827A8(SteamKnightPart *body) { (void)body; abort(); }
void func_8012B23C(s32 *body) { (void)body; abort(); }

s32 func_8012CBA4(SteamKnightPart *body)
{
    assert(body == active);
    event('U');
    animation(next_motion, next_frame);
    return 0x8000;
}

void bfm_steam_knight_transition_script(u32 descriptor)
{
    assert(descriptor == 0x80191060u);
    event('C');
    animation(7, -2);
}

void func_8002D4C8(u16 cue, u16 argument)
{
    assert(argument == 0 && (cue == 1265 || cue == 1291));
    event(cue == 1265 ? 'a' : 'b');
}

void func_8018655C(SteamKnightPart *body)
{
    assert(body == active);
    event('T');
    if (tail_clears_flag)
        body->phase_flags &= ~0x1000u;
}

void func_80182998(SteamKnightPart *body)
{
    assert(body == active);
    event('B');
}

static int predicate(SteamKnightPart *body, int index)
{
    assert(body == active);
    event((char)('0' + index));
    /* A successful predicate sets the bit that the dispatcher must clear. */
    if (trigger == index) {
        active->phase_flags |= 0x1000;
        return -7;
    }
    return 0;
}

int func_8018687C(SteamKnightPart *body) { return predicate(body, 1); }
int func_8018675C(SteamKnightPart *body) { return predicate(body, 2); }
int func_8012BEE8(SteamKnightPart *body) { return predicate(body, 3); }

static void check_countdown(int flip)
{
    reset(&actor);
    flip_speed = flip;
    actor.break_count = 0x8000;
    actor.phase_flags = 0x880;
    func_801838A8(&actor);
    assert(strcmp(events, "JMSF") == 0 && actor.break_count == 0x7fff);
    assert(actor.motion_x == -0x1c000 && actor.motion_z == 0 && actor.field_e0 == -123);
    assert(selected_motion == -3 && state_timer == (flip ? 24 : 48) && actor.timer == state_timer);
    assert(actor.phase_flags == (0x5841u | (flip ? 0x2000u : 0u)));
}

static void check_animation(s32 motion, s16 frame, const char *order)
{
    reset(&actor);
    animation(13, 1);
    next_motion = motion;
    next_frame = frame;
    func_80182918(&actor);
    assert(strcmp(events, order) == 0);
}

static void check_predicates(int winner, const char *order)
{
    reset(&actor);
    actor.phase_flags = 0x1000;
    actor.break_delay = 3;
    trigger = winner;
    decision = -3;
    tail_clears_flag = winner != 0;
    func_8018791C(&actor);
    assert(strcmp(events, order) == 0);
    assert(actor.break_delay == (winner ? 2 : 3));
    assert(actor.phase_flags == (winner ? 0u : 0x1000u));
}

int main(void)
{
    check_countdown(0);
    check_countdown(1);
#ifdef TEST_ALIASED_MODE
    reset(&overlapping_actor);
    overlapping_actor.phase_flags = 1;
    func_80182810(&overlapping_actor);
    assert(strcmp(events, "M") == 0 && selected_motion == -3);
    assert(overlapping_actor.phase_flags == 0x1001 && overlapping_actor.field_e0 == -1);
#endif
    check_animation(1, 1, "UCabT");
    check_animation(13, 1, "UCabT");
    check_animation(0x10001, 1, "UT");
    check_animation(13, -1, "UT");
    check_predicates(0, "UT123");
    check_predicates(1, "UT1QLI");
    check_predicates(2, "UT12QLI");
    check_predicates(3, "UT123QLI");

    reset(&actor);
    actor.phase_flags = 0x40000;
    actor.break_delay = 5;
    func_8018791C(&actor);
    assert(strcmp(events, "B") == 0 && actor.break_delay == 5);
    return 0;
}

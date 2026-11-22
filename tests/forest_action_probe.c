#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "forest_action.h"

static ForestPlayerState player;
static const u32 actor_address = 0x80122480u;
enum Operation {
    TEST_TRANSITION,
    RUN_KIND_HANDLER,
    FINISH_TRANSITION,
    SAMPLE,
    SET_SEQUENCE,
    CHECK_RESET,
    LOAD_VECTOR,
    RESTART_ANIMATION,
    SELECT_ANIMATION_0,
    SELECT_ANIMATION_1,
    SELECT_ANIMATION_2,
    SELECT_ANIMATION_3,
    UPDATE_FRAME,
    UPDATE_EFFECT,
    RESOLVE_ACTOR,
    ISSUE_REQUEST,
    CHECK_FINAL_ACTION,
};

static enum Operation events[40];
static size_t count;
static s32 initial_condition, sample, next_step;
u16 D_800B99DA;

static void observe(enum Operation event, ForestActionPlayer *actor)
{
    assert(actor == &player && count < sizeof events / sizeof events[0]);
    events[count++] = event;
}

static void expect(const enum Operation *wanted, size_t length)
{
    assert(count == length);
    assert(memcmp(events, wanted, count * sizeof events[0]) == 0);
}

static void reset(void)
{
    memset(&player, 0, sizeof player);
    initial_condition = 0;
    count = 0;
    D_800B99DA = 3;
    sample = 1;
    next_step = -1;
}

s32 func_80149D9C(ForestActionPlayer *actor) { observe(TEST_TRANSITION, actor); return initial_condition; }
u32 bfm_forest_player_table_word(u32 table, u32 index)
{
    assert(table == 0x801891b8u && index == 0x8123);
    return 0x80167890u;
}
void bfm_forest_player_call(u32 address, ForestPlayerState *actor)
{
    assert(address == 0x80167890u);
    observe(RUN_KIND_HANDLER, actor);
    actor->action = 9;
}
void func_80149FA8(ForestActionPlayer *actor) { observe(FINISH_TRANSITION, actor); assert(actor->action == 9); }

#define ZERO_PREDICATE(name) \
    s32 name(ForestActionPlayer *actor) { assert(actor == &player); return 0; }
ZERO_PREDICATE(func_80149FB0)
ZERO_PREDICATE(func_80161B18)
ZERO_PREDICATE(func_80161B84)
ZERO_PREDICATE(func_80161BE0)
ZERO_PREDICATE(func_801496D4)
ZERO_PREDICATE(func_80149CD4)
ZERO_PREDICATE(func_801488A8)
s32 func_801619A4(ForestActionPlayer *actor) { assert(actor == &player); return 7; }
s32 func_80161208(ForestActionPlayer *actor) { observe(CHECK_FINAL_ACTION, actor); return 1; }

s32 bfm_forest_action_sample(void) { observe(SAMPLE, &player); return sample; }
s32 func_80154B4C(ForestActionPlayer *actor, u32 address)
{
    observe(SET_SEQUENCE, actor);
    assert(address == 0x800d5f14u);
    actor->sequence = address;
    if (next_step >= 0)
        actor->step = (u32)next_step;
    return 0;
}
u32 func_8016F1AC(void) { observe(CHECK_RESET, &player); return 0; }
s32 bfm_forest_action_vector(ForestActionPlayer *actor, u32 address)
{
    observe(LOAD_VECTOR, actor);
    assert(address == 0x8018911cu);
    return 0;
}
s32 func_80154190(ForestActionPlayer *actor, s32 index)
{
    observe(RESTART_ANIMATION, actor);
    assert(index == 0);
    return 0;
}
s32 func_80154150(ForestActionPlayer *actor, s32 index)
{
    assert(index >= 0 && index <= 3);
    observe((enum Operation)(SELECT_ANIMATION_0 + index), actor);
    return 0;
}
void func_80147A84(ForestActionPlayer *actor) { observe(UPDATE_FRAME, actor); }
s32 func_801473EC(ForestActionPlayer *actor) { observe(UPDATE_EFFECT, actor); return 0; }
u32 bfm_forest_action_actor_address(ForestPlayerState *actor)
{
    observe(RESOLVE_ACTOR, actor);
    return actor_address;
}
s32 func_8014C010(s32 address, s32 request)
{
    assert((u32)address == actor_address && request == 1);
    observe(ISSUE_REQUEST, &player);
    return 0;
}

#define UNEXPECTED_RESULT(name) s32 name(ForestActionPlayer *actor) { (void)actor; abort(); }
#define UNEXPECTED_CALL(name) void name(ForestActionPlayer *actor) { (void)actor; abort(); }
UNEXPECTED_RESULT(func_80149AA8)
UNEXPECTED_RESULT(func_80149B54)
UNEXPECTED_RESULT(func_8014A738)
UNEXPECTED_RESULT(func_80149A64)
UNEXPECTED_RESULT(func_80149704)
UNEXPECTED_RESULT(func_8015A230)
UNEXPECTED_RESULT(func_801483E8)
UNEXPECTED_RESULT(func_80172590)
UNEXPECTED_CALL(func_80149AD4)
UNEXPECTED_CALL(func_8014A830)
UNEXPECTED_CALL(func_80149BAC)
UNEXPECTED_CALL(func_801498E0)
UNEXPECTED_CALL(func_80149954)
UNEXPECTED_CALL(func_800CCCC0)
UNEXPECTED_CALL(func_8015E880)
UNEXPECTED_CALL(func_80155440)
s32 func_80147324(u16 request) { (void)request; abort(); }
s32 func_80154A74(ForestActionPlayer *actor, s32 value) { (void)actor; (void)value; abort(); }

int main(void)
{
    static const enum Operation early[] = {
        TEST_TRANSITION, RUN_KIND_HANDLER, FINISH_TRANSITION
    };
    static const enum Operation vector[] = {
        TEST_TRANSITION, SAMPLE, CHECK_RESET, LOAD_VECTOR, RESTART_ANIMATION, UPDATE_FRAME, UPDATE_EFFECT, RESOLVE_ACTOR, ISSUE_REQUEST, CHECK_FINAL_ACTION
    };
    static const enum Operation reset_step[] = {
        TEST_TRANSITION, SAMPLE, SET_SEQUENCE, CHECK_RESET, SELECT_ANIMATION_0, CHECK_FINAL_ACTION
    };
    static const enum Operation advance[] = {
        TEST_TRANSITION, CHECK_RESET, SELECT_ANIMATION_2, CHECK_FINAL_ACTION
    };
    static const enum Operation wrapped[] = {
        TEST_TRANSITION, CHECK_RESET, RESTART_ANIMATION, CHECK_FINAL_ACTION
    };

    reset();
    initial_condition = 1;
    player.kind = 0x8123;
    func_80159C84(&player);
    expect(early, sizeof early / sizeof early[0]);

    reset();
    player.animation = 0x800d49f4u;
    player.animation_flags = player.sequence_flags = 0x8000;
    player.counter = 3;
    D_800B99DA = 4;
    sample = 0x21;
    func_80159C84(&player);
    expect(vector, sizeof vector / sizeof vector[0]);
    assert(player.counter == 4 && player.step == 0 && player.sequence == 0);

    reset();
    player.animation_flags = player.sequence_flags = 0x8000;
    player.flags = 4;
    player.counter = 2;
    sample = 0x10;
    next_step = 2;
    func_80159C84(&player);
    expect(reset_step, sizeof reset_step / sizeof reset_step[0]);
    assert(player.sequence == 0x800d5f14u && player.step == 0 && player.counter == 0);

    reset();
    player.animation_flags = 0x8000;
    player.step = 2;
    player.counter = 2;
    func_80159C84(&player);
    expect(advance, sizeof advance / sizeof advance[0]);
    assert(player.step == 3 && player.counter == 0);

    reset();
    player.animation_flags = 0x8000;
    player.step = 2;
    player.counter = 0xffffffffu;
    func_80159C84(&player);
    expect(wrapped, sizeof wrapped / sizeof wrapped[0]);
    assert(player.step == 2 && player.counter == 0);
    return 0;
}

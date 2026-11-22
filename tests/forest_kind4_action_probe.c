#include "kind4_action.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

s32 func_8015BFF4(ForestPlayerState *actor);
s32 func_8015C128(ForestPlayerState *actor);

enum Outcome { EVENT, CHANGE_ACTION, SLOPE_STOP, SLOPE_RESTART, FLOOR_KEEP, FLOOR_RELEASE };
enum Event {
    UPDATE, INPUT, IMPULSE, ROTATE, SET_VECTOR, MOVE, INTEGRATE, EFFECT, SOUND,
    CHECK_EVENT, FLOOR, CONTACT, LOOKUP_KIND, DISPATCH_KIND, FINISH_EVENT,
    SET_ACTION, FINISH_ACTION, CHECK_SLOPE, SET_KIND, RESTART_SLOPE, RELEASE_FLOOR
};

static ForestPlayerState player;
static u8 model[0x14];
static enum Outcome outcome;
static enum Event calls[24];
static unsigned count;
static int direction, rotating_impulse, selecting_action;
static s16 heading;
static s32 dispatch_result;
static unsigned selector_calls;
u16 D_800B99DA;

static void note(enum Event event)
{
    assert(count < sizeof(calls) / sizeof(calls[0]));
    calls[count++] = event;
}

static void visit(ForestPlayerState *actor, enum Event event)
{
    assert(actor == &player);
    note(event);
}

u8 *bfm_forest_orientation_bytes(u32 handle, u32 size, enum ForestOrientationRole role)
{
    assert(handle == player.model && size == sizeof(model) && role == FOREST_ORIENTATION_SUBOBJECT);
    return model;
}

_Noreturn void bfm_forest_ground_attachment_missing(u32 handle) { (void)handle; abort(); }
u32 bfm_forest_action_actor_address(ForestPlayerState *actor) { assert(actor == &player); return 39; }

u32 bfm_forest_player_table_word(u32 table, u32 index)
{
    if (selecting_action) {
        assert(table == 0x80189338 && index == player.action);
        ++selector_calls;
    } else {
        assert(table == 0x801891b8 && index == player.kind);
        note(LOOKUP_KIND);
    }
    return 123u + index;
}

s32 bfm_forest_player_call_result(u32 target, ForestPlayerState *actor)
{
    assert(selecting_action && actor == &player && target == 123u + actor->action);
    ++selector_calls;
    return dispatch_result;
}

void bfm_forest_player_call(u32 target, ForestPlayerState *actor)
{
    assert(!selecting_action && target == 123u + actor->kind);
    visit(actor, DISPATCH_KIND);
}

s32 func_80149FB0(ForestPlayerState *actor) { visit(actor, UPDATE); return 0; }
s32 func_80148AFC(ForestPlayerState *actor) { visit(actor, INPUT); return direction; }

void func_8001382C(s16 yaw, const void *vector, s32 *result)
{
    u32 v[3];
    memcpy(v, vector, sizeof(v));
    if (rotating_impulse) {
        assert(yaw == heading && !v[0] && !v[1] && v[2] == (u32)-0x4000);
        note(IMPULSE);
        result[0] = 7; result[1] = -3; result[2] = -9;
        rotating_impulse = 0;
    } else {
        assert((u16)yaw == (u16)(0u - (u16)heading));
        assert(v[0] == ((direction & 255) ? 5u : 0xfffffffeu));
        assert(v[1] == ((direction & 255) ? 5u : 8u));
        assert(v[2] == ((direction & 255) ? (u32)-9 : 0u));
        assert(v[0] == player.step && v[1] == player.counter && v[2] == player.field_23c);
        note(ROTATE);
        result[0] = -11; result[1] = 13; result[2] = 17;
    }
}

s32 func_80146DB8(s32 *actor, s32 *v)
{
    assert((void *)actor == &player && v[0] == -11 && v[1] == 13 && v[2] == 17);
    note(SET_VECTOR);
    return 0;
}

void func_80147A84(ForestPlayerState *actor) { visit(actor, MOVE); }
s32 func_801473EC(ForestPlayerState *actor) { visit(actor, INTEGRATE); return 0; }
s32 func_8014C010(s32 handle, s32 kind) { assert(handle == 39 && kind == 1); note(EFFECT); return 0; }
s32 func_80147324(u16 id) { assert(id == 0x65f); note(SOUND); return 0; }
s32 func_8014D738(ForestPlayerState *actor) { visit(actor, CHECK_EVENT); return outcome == EVENT; }
s32 func_8015C6E0(ForestPlayerState *actor) { visit(actor, FINISH_EVENT); return -31; }
s32 func_8014CC28(ForestPlayerState *actor) { visit(actor, FLOOR); return outcome == FLOOR_KEEP; }

s32 func_8014F3E8(ForestPlayerState *actor)
{
    visit(actor, CONTACT);
    if (outcome == CHANGE_ACTION) return 0x1213;
    if (outcome == SLOPE_STOP || outcome == SLOPE_RESTART) return 0x7400d;
    return 0;
}

void func_80146E90(ForestPlayerState *actor, s32 value) { assert(value == 6); visit(actor, SET_ACTION); }
s32 func_80146CA0(ForestPlayerState *actor) { visit(actor, FINISH_ACTION); return 57; }
s32 func_80146E98(ForestPlayerState *actor) { visit(actor, CHECK_SLOPE); return outcome == SLOPE_RESTART; }

void func_80147078(ForestPlayerState *actor, u16 kind)
{
    assert(kind == (outcome == SLOPE_RESTART ? 4u : 3u));
    visit(actor, SET_KIND);
}

s32 func_8015C0C4(ForestPlayerState *actor) { visit(actor, RESTART_SLOPE); return -97; }
s32 func_8015BE94(ForestPlayerState *actor) { visit(actor, RELEASE_FLOOR); return 83; }

static void expect(unsigned *i, enum Event event) { assert(*i < count && calls[(*i)++] == event); }

static void test_action(enum Outcome branch, int input, u16 tick)
{
    static const s32 result[] = {-31, 57, 0, -97, 0, 83};
    player = (ForestPlayerState){0};
    player.kind = 7;
    player.model = 73;
    player.step = 0xfffffffeu;
    player.counter = 8;
    outcome = branch;
    direction = input;
    rotating_impulse = !!(direction & 255);
    heading = tick ? -32768 : 619;
    memcpy(model + 0x12, &heading, sizeof(heading));
    D_800B99DA = tick;
    count = 0;
    assert(func_8015C128(&player) == result[outcome]);
    unsigned i = 0;
    expect(&i, UPDATE); expect(&i, INPUT);
    if (input & 255) expect(&i, IMPULSE);
    expect(&i, ROTATE); expect(&i, SET_VECTOR); expect(&i, MOVE); expect(&i, INTEGRATE);
    if (!(tick & 3)) { expect(&i, EFFECT); expect(&i, SOUND); }
    expect(&i, CHECK_EVENT);
    if (branch == EVENT) {
        expect(&i, LOOKUP_KIND); expect(&i, DISPATCH_KIND); expect(&i, FINISH_EVENT);
    } else {
        expect(&i, FLOOR); expect(&i, CONTACT);
        if (branch == CHANGE_ACTION) { expect(&i, SET_ACTION); expect(&i, FINISH_ACTION); }
        if (branch == SLOPE_STOP || branch == SLOPE_RESTART) expect(&i, CHECK_SLOPE);
        if (branch == SLOPE_RESTART) { expect(&i, SET_KIND); expect(&i, RESTART_SLOPE); }
        if (branch == FLOOR_RELEASE) {
            expect(&i, LOOKUP_KIND); expect(&i, DISPATCH_KIND); expect(&i, SET_KIND); expect(&i, RELEASE_FLOOR);
        }
    }
    assert(i == count);
}

int main(void)
{
    const u16 indices[] = {0, 1, 2, 0x8000, 0xffff};
    selecting_action = 1;
    for (unsigned i = 0; i < sizeof(indices) / sizeof(indices[0]); ++i) {
        player.action = indices[i];
        dispatch_result = i & 1u ? -937 : 528;
        selector_calls = 0;
        assert(func_8015BFF4(&player) == dispatch_result && selector_calls == 2);
    }
    selecting_action = 0;
    for (int i = EVENT; i <= FLOOR_RELEASE; ++i)
        for (unsigned tick = 0; tick < 4; ++tick) {
            test_action((enum Outcome)i, 0x100, (u16)tick);
            test_action((enum Outcome)i, 0x101, (u16)tick);
        }
    return 0;
}

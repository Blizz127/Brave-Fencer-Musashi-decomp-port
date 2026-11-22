#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "steam_knight_hit.h"

void func_80183218(SteamKnightPart *body);

_Static_assert(sizeof(SteamKnightPart) == 0x10c, "actor record size");
_Static_assert(offsetof(SteamKnightPart, state) == 2, "body state");
_Static_assert(offsetof(SteamKnightPart, model_address) == 0x20, "model handle");
_Static_assert(offsetof(SteamKnightPart, display_record) == 0x58, "display record");
_Static_assert(offsetof(SteamKnightPart, damage) == 0x60, "pending damage");
_Static_assert(offsetof(SteamKnightPart, hp) == 0x76, "body health");
_Static_assert(offsetof(SteamKnightPart, linked_address) == 0xcc, "linked actor");
_Static_assert(offsetof(SteamKnightPart, secondary_address) == 0xd0, "secondary actor");
_Static_assert(offsetof(SteamKnightPart, core_address) == 0xd4, "core actor");
_Static_assert(offsetof(SteamKnightPart, field_dc) == 0xdc, "mode and countdown");
_Static_assert(offsetof(SteamKnightPart, field_e0) == 0xe0, "reaction result");
_Static_assert(offsetof(SteamKnightPart, phase_flags) == 0xe8, "phase flags");

s16 D_801E4398;
u16 D_801E4BBC;

enum Actor { LINK = 1, ALT_LINK, PART, ALT_PART, CORE, ALT_CORE, ACTOR_COUNT };
enum Model { MODEL = 1, ALT_MODEL, COUNTDOWN_MODEL };
enum PhaseChange { KEEP_PHASE, SELECT_SET, SELECT_CLEAR };
enum Event {
    HIT_SOUND, CORE_HIT, RESET, BODY_EFFECT, PART_EFFECT, STATE, SEQUENCE,
    BODY_FINISH, GROUP_FINISH, ROUND_ONE, ROUND_TWO, GROUP_RESET, ROUND_EFFECT,
    RECOIL, END_SOUND, EXPIRED
};

static SteamKnightPart body, actors[ACTOR_COUNT];
static u8 models[2][32];
static enum Event log_events[24];
static size_t num_events;
static struct {
    enum PhaseChange phase;
    int revive, retarget, countdown_model, change_round, set_recoil, arm_timer;
    u16 new_round;
} callbacks;

static void record(enum Event event)
{
    assert(num_events < sizeof log_events / sizeof log_events[0]);
    log_events[num_events++] = event;
}

static void expect(const enum Event *events, size_t size)
{
    assert(num_events * sizeof *events == size);
    assert(memcmp(events, log_events, size) == 0);
}

#define EXPECT(...) expect((const enum Event[]){__VA_ARGS__}, sizeof((const enum Event[]){__VA_ARGS__}))

static u16 half(size_t offset)
{
    u16 value;
    memcpy(&value, (const u8 *)&body + offset, sizeof value);
    return value;
}

static void put_half(size_t offset, u16 value)
{
    memcpy((u8 *)&body + offset, &value, sizeof value);
}

static void setup(void)
{
    memset(&body, 0, sizeof body);
    memset(actors, 0, sizeof actors);
    memset(models, 0xa7, sizeof models);
    memset(&callbacks, 0, sizeof callbacks);
    num_events = 0;
    body.state = 1;
    body.hp = 6;
    body.damage = 6;
    body.phase_flags = 0x000e0011;
    body.linked_address = LINK;
    body.secondary_address = PART;
    body.core_address = CORE;
    actors[LINK].model_address = MODEL;
    actors[ALT_LINK].model_address = ALT_MODEL;
    body.field_e0 = 127;
    D_801E4398 = -71;
    D_801E4BBC = 0;
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 handle)
{
    assert(handle >= LINK && handle < ACTOR_COUNT);
    return &actors[handle];
}

void *bfm_tower_spawn_pointer(u32 handle)
{
    if (handle == COUNTDOWN_MODEL)
        return (u8 *)&body + 0xcc;
    assert(handle >= MODEL && handle <= ALT_MODEL);
    return models[handle - MODEL];
}

void func_8002D4C8(u16 command, u16 argument)
{
    assert(argument == 0);
    if (command == 0x4f3) {
        record(HIT_SOUND);
        assert(body.damage == 0 && D_801E4398 == body.hp);
        assert(!(body.phase_flags & 0x10));
        if (callbacks.revive)
            body.hp = 13;
    } else {
        assert(command == 0x50f);
        record(END_SOUND);
        if (callbacks.arm_timer)
            put_half(0xde, 1);
    }
}

void func_8018415C(SteamKnightPart *core)
{
    record(CORE_HIT);
    assert(core == &actors[CORE]);
    assert(body.hp == 0 && !(body.phase_flags & 0xe0000));
    if (callbacks.phase == SELECT_SET)
        body.phase_flags |= 8;
    if (callbacks.phase == SELECT_CLEAR)
        body.phase_flags &= ~8u;
}

void func_8012B23C(s32 *actor)
{
    assert((void *)actor == (void *)&body);
    record(RESET);
    if (callbacks.retarget)
        body.phase_flags |= 0x20001;
}

void func_80186F58(SteamKnightPart *actor)
{
    assert(actor == &body);
    record(BODY_EFFECT);
    if (callbacks.retarget) {
        body.linked_address = ALT_LINK;
        body.secondary_address = ALT_PART;
        assert((body.phase_flags & 0x20001) == 0x20000);
    }
}

void func_80184C20(SteamKnightPart *part)
{
    int index = callbacks.retarget ? 1 : 0;
    record(PART_EFFECT);
    assert(part == &actors[callbacks.retarget ? ALT_PART : PART]);
    if (callbacks.countdown_model) {
        assert(half(0xde) == 0);
    } else {
        assert(models[index][17] == 0xa7 && models[index][20] == 0xa7);
        assert(models[index][18] == 0 && models[index][19] == 0);
        assert(models[1 - index][18] == 0xa7 && models[1 - index][19] == 0xa7);
    }
}

void func_8012AD44(u8 *actor, u16 state)
{
    assert(actor == (u8 *)&body);
    record(STATE);
    if (body.phase_flags & 8) {
        assert(state == 8 && body.flags == 0xa00);
        assert((body.display_record & 0x40000000u) && (body.phase_flags & 4));
    } else {
        assert(state == 10);
        if (callbacks.change_round)
            D_801E4BBC = callbacks.new_round;
    }
}

void func_80186304(SteamKnightPart *actor, int sequence, int speed)
{
    assert(actor == &body);
    record(SEQUENCE);
    if (body.phase_flags & 8) {
        assert(sequence == 10 && speed == 2);
    } else {
        assert(sequence == 12 && speed == 3);
        assert(body.motion_z == 0xa8000 && body.motion_y == -0xc0000);
        assert(half(0xdc) == 2);
    }
}

void func_80186BB8(SteamKnightPart *actor)
{
    assert(actor == &body);
    record(BODY_FINISH);
}

void func_80186D00(void)
{
    record(GROUP_FINISH);
    if (callbacks.change_round)
        D_801E4BBC = callbacks.new_round;
    if (callbacks.retarget)
        body.core_address = ALT_CORE;
}

void func_801900F4(void) { record(ROUND_ONE); }

void func_80184184(SteamKnightPart *core)
{
    assert(core == &actors[callbacks.retarget ? ALT_CORE : CORE]);
    assert(body.flags == 0xa00);
    body.flags = 123;
    record(ROUND_TWO);
}

void func_8018AAAC(void) { record(GROUP_RESET); }

void func_8017DF50(void)
{
    record(ROUND_EFFECT);
    if (callbacks.set_recoil) {
        body.phase_flags |= 0x800;
        body.motion_z = 1234567;
    }
}

s16 func_80186144(SteamKnightPart *actor, int mode)
{
    record(RECOIL);
    assert(actor == &body && mode == 5);
    assert(body.motion_x == -1234567 && body.motion_z == 0);
    return -1234;
}

void func_80186ADC(SteamKnightPart *actor)
{
    assert(actor == &body && half(0xde) == 0);
    record(EXPIRED);
}

static void check_gates_and_countdown(void)
{
    static const u16 ticks[][2] = {{0, 0}, {1, 0}, {2, 1}, {0x8000, 0x7fff}, {0xffff, 0xfffe}};
    for (unsigned gate = 0; gate < 2; ++gate) {
        for (size_t i = 0; i < sizeof ticks / sizeof ticks[0]; ++i) {
            setup();
            if (gate) body.state = 0;
            else body.phase_flags &= ~0x10u;
            put_half(0xdc, 0x1234);
            put_half(0xde, ticks[i][0]);
            func_80183218(&body);
            assert(body.hp == 6 && body.damage == 6 && D_801E4398 == -71);
            assert(half(0xdc) == 0x1234 && half(0xde) == ticks[i][1]);
            if (ticks[i][0] == 1) { EXPECT(EXPIRED); }
            else assert(num_events == 0);
        }
    }
}

static void check_damage(void)
{
    static const s16 amounts[][3] = {{53, 9, 44}, {5, -3, 8}, {-32768, 1, 32767}, {2, 7, 0}, {32767, -1, 0}};
    for (size_t i = 0; i < sizeof amounts / sizeof amounts[0]; ++i) {
        setup();
        body.hp = amounts[i][0];
        body.damage = amounts[i][1];
        callbacks.revive = amounts[i][2] == 0;
        put_half(0xde, 1);
        func_80183218(&body);
        assert(D_801E4398 == amounts[i][2]);
        assert(body.hp == (callbacks.revive ? 13 : D_801E4398));
        assert(body.phase_flags == 0x000e0001 && body.damage == 0);
        EXPECT(HIT_SOUND, EXPIRED);
    }
}

static void check_set_phase(void)
{
    for (u16 round = 0; round <= 2; ++round) {
        setup();
        callbacks.phase = SELECT_SET;
        callbacks.retarget = 1;
        callbacks.change_round = 1;
        callbacks.new_round = round;
        D_801E4BBC = 7;
        func_80183218(&body);
        assert(D_801E4398 == 0 && body.phase_flags == 0x2000c);
        assert(body.flags == (round == 2 ? 0x800 : 0xa00));
        if (round == 1) {
            EXPECT(HIT_SOUND, CORE_HIT, RESET, BODY_EFFECT, PART_EFFECT, STATE, SEQUENCE, BODY_FINISH, GROUP_FINISH, ROUND_ONE);
        } else if (round == 2) {
            EXPECT(HIT_SOUND, CORE_HIT, RESET, BODY_EFFECT, PART_EFFECT, STATE, SEQUENCE, BODY_FINISH, GROUP_FINISH, ROUND_TWO);
        } else {
            EXPECT(HIT_SOUND, CORE_HIT, RESET, BODY_EFFECT, PART_EFFECT, STATE, SEQUENCE, BODY_FINISH, GROUP_FINISH);
        }
    }
}

static void check_clear_phase(void)
{
    for (int variant = 0; variant < 3; ++variant) {
        setup();
        body.phase_flags |= 8;
        callbacks.phase = SELECT_CLEAR;
        callbacks.change_round = 1;
        callbacks.new_round = variant ? 2 : 7;
        callbacks.set_recoil = variant == 2;
        callbacks.countdown_model = variant == 0;
        if (callbacks.countdown_model) actors[LINK].model_address = COUNTDOWN_MODEL;
        callbacks.arm_timer = variant == 2;
        put_half(0xde, callbacks.arm_timer ? 5 : 1);
        func_80183218(&body);
        assert(body.hp == 0 && D_801E4398 == 0 && half(0xde) == 0);
        assert(body.motion_y == -0xc0000 && half(0xdc) == 2);
        assert(body.field_e0 == (callbacks.set_recoil ? -1234 : 127));
        if (variant == 0) {
            EXPECT(HIT_SOUND, CORE_HIT, PART_EFFECT, GROUP_RESET, BODY_EFFECT, RESET, SEQUENCE, STATE, BODY_FINISH, END_SOUND);
        } else if (variant == 1) {
            EXPECT(HIT_SOUND, CORE_HIT, PART_EFFECT, GROUP_RESET, BODY_EFFECT, RESET, SEQUENCE, STATE, ROUND_EFFECT, BODY_FINISH, END_SOUND, EXPIRED);
        } else {
            EXPECT(HIT_SOUND, CORE_HIT, PART_EFFECT, GROUP_RESET, BODY_EFFECT, RESET, SEQUENCE, STATE, ROUND_EFFECT, RECOIL, BODY_FINISH, END_SOUND, EXPIRED);
        }
    }
}

int main(void)
{
    check_gates_and_countdown();
    check_damage();
    check_set_phase();
    check_clear_phase();
    return 0;
}

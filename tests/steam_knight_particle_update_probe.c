#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "steam_knight_actor84.h"

_Static_assert(offsetof(SteamKnightPart, position_y) == 10, "signed Y");
_Static_assert(offsetof(SteamKnightPart, model_address) == 32, "model token");
_Static_assert(offsetof(SteamKnightPart, field_fc) == 252, "first increment");
_Static_assert(offsetof(SteamKnightPart, break_count) == 254, "second increment");

static SteamKnightPart part;
static u8 model[32];
static u32 expected_tokens[2];
static s16 post_motion_y;
static unsigned calls, state_reads, dispatches;
static u32 state_word;
static int redirected;

static void put_half(u8 *field, u16 value)
{
    memcpy(field, &value, sizeof value);
}

static u16 half(const u8 *field)
{
    u16 value;
    memcpy(&value, field, sizeof value);
    return value;
}

void *bfm_tower_spawn_pointer(u32 address)
{
    if (address == 0x801966bcu) {
        assert(state_reads++ == 0 && calls == 0);
        return &state_word;
    }
    assert(dispatches == 1 && calls < 2 && address == expected_tokens[calls]);
    ++calls;
    if (redirected && calls == 1) {
        /* The first store at model+18 lands on the low half of actor.model_address. */
        return (u8 *)&part + offsetof(SteamKnightPart, model_address) - 18;
    }
    return model;
}

void bfm_tower_spawn_setup(u32 target, void *actor)
{
    assert(dispatches++ == 0 && state_reads == 1 && actor == &part);
    assert(target == state_word);
    func_8018BAE4(&part);
}

void func_8012AD80(SteamKnightPart *actor)
{
    assert(calls++ == 2 && actor == &part);
    assert(half(model + 16) == 9);
    if (redirected) assert(actor->model_address == expected_tokens[1]);
    else assert(half(model + 18) == 5);
    actor->position_y = post_motion_y;
}

s32 func_8012C218(u8 *actor)
{
    assert(calls++ == 3 && actor == (u8 *)&part && part.position_y >= -2047);
    part.active = 0;
    return -31;
}

static void check_update(int overlap, s16 y_before, s16 y_after)
{
    SteamKnightPart expected;
    u8 expected_model[sizeof model];
    memset(&part, 0x31, sizeof part);
    memset(model, 0x63, sizeof model);
    part.state = 1;
    part.position_y = y_before;
    part.model_address = 0x94200404u;
    part.field_fc = overlap ? 8 : 9;
    part.break_count = 14;
    put_half(model + 16, 65531);
    put_half(model + 18, 65532);
    redirected = overlap;
    post_motion_y = y_after;
    calls = state_reads = dispatches = 0;
    state_word = overlap ? 0xe0110020u : 0xa7740050u;
    expected_tokens[0] = part.model_address;
    expected_tokens[1] = expected_tokens[0] + (overlap ? part.field_fc : 0);
    memcpy(&expected, &part, sizeof part);
    memcpy(expected_model, model, sizeof model);
    expected.model_address = expected_tokens[1];
    expected.position_y = y_after;
    if (y_after >= -2047) expected.active = 0;
    put_half(expected_model + 16, 9);
    if (!overlap) put_half(expected_model + 18, 5);

    func_80189CCC(&part);
    assert(state_reads == 1 && dispatches == 1);
    assert(calls == (y_after >= -2047 ? 4u : 3u));
    assert(memcmp(&part, &expected, sizeof part) == 0);
    assert(memcmp(model, expected_model, sizeof model) == 0);
}

int main(void)
{
    check_update(0, -2047, -2048);
    check_update(0, -2048, -2047);
    check_update(1, -2046, -32768);
    check_update(1, -32768, 32767);
    return 0;
}

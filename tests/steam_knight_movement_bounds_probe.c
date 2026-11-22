#include <assert.h>
#include <string.h>
#include "steam_knight_hit.h"

s32 func_8018675C(SteamKnightPart *body);
s32 func_8018687C(SteamKnightPart *body);

static SteamKnightPart body;
static u16 round_index, count;
static s32 route_value;
static s16 left_edge, bounds[3][2], z_limit[3], markers[3][4];
static void *count_address = &count;
static void *value_address = &route_value;

void *bfm_tower_spawn_pointer(u32 address)
{
    unsigned i;
    switch (address) {
    case 0x801e4bbcu: return &round_index;
    case 0x801e43b4u: return count_address;
    case 0x801270c8u: return value_address;
    case 0x80195aceu: return &left_edge;
    }
    for (i = 0; i < 3; i++) {
        if (address == 0x80195ad0u + 4 * i) return &bounds[i][0];
        if (address == 0x80195ad2u + 4 * i) return &bounds[i][1];
        if (address == 0x80195ac4u + 4 * i) return &z_limit[i];
        if (address == 0x80195af4u + 8 * i) return &markers[i][0];
        if (address == 0x80195afau + 8 * i) return &markers[i][3];
    }
    assert(0);
    return 0;
}

static void set_x(s16 x)
{
    memcpy((u8 *)&body + 6, &x, sizeof x);
}

static s16 get_x(void)
{
    s16 x;
    memcpy(&x, (u8 *)&body + 6, sizeof x);
    return x;
}

static void reset(void)
{
    memset(&body, 0, sizeof body);
    body.break_count = 31;
    body.field_e0 = -1;
    count = 27;
    route_value = 513;
    count_address = &count;
    value_address = &route_value;
}

static void check_walls(void)
{
    int direction, delta, phase, upper;
    for (round_index = 0; round_index < 3; round_index++) {
        bounds[round_index][0] = (s16)(-180 - 13 * round_index);
        bounds[round_index][1] = (s16)(119 + 17 * round_index);
        for (upper = 0; upper <= 1; upper++) {
            s16 edge = bounds[round_index][upper];
            for (direction = -1; direction <= 1; direction++) {
                for (delta = -1; delta <= 1; delta++) {
                    for (phase = 0; phase < 2; phase++) {
                        s16 before = (s16)(edge + delta);
                        int clamp = !phase && ((direction > 0 && upper && delta > 0) ||
                                              (direction < 0 && !upper && delta < 0));
                        reset();
                        body.phase_flags = 0x20u | (phase ? 0x800u : 0u);
                        body.motion_x = direction;
                        set_x(before);
                        assert(func_8018687C(&body) == clamp);
                        assert(get_x() == (clamp ? edge : before));
                        assert(body.break_count == 31 && body.motion_x == direction);
                    }
                }
            }
        }
    }
}

static void check_depth(void)
{
    int direction, delta;
    for (round_index = 0; round_index < 3; round_index++) {
        z_limit[round_index] = (s16)(-61 + 67 * round_index);
        for (direction = -1; direction <= 1; direction++) {
            for (delta = -1; delta <= 1; delta++) {
                int crossed = direction > 0 && delta > 0;
                u32 flag = round_index == 2 ? 0x400u : 8u;
                reset();
                body.phase_flags = 0x10000;
                body.motion_z = direction;
                body.field_0e = (s16)(z_limit[round_index] + delta);
                assert(func_8018675C(&body) == crossed);
                assert(body.phase_flags == (0x10000u | (crossed ? flag : 0u)));
                assert(body.break_count == (crossed ? 0 : 31));
            }
        }
    }
}

static void check_markers(void)
{
    int index, x, direction;
    left_edge = -18;
    for (index = 0; index < 3; index++) {
        markers[index][0] = (s16)(-9 + 13 * index);
        markers[index][3] = (s16)(-30000 + 20000 * index);
    }
    for (index = -1; index < 3; index++) {
        for (x = -19; x <= 18; x++) {
            for (direction = -1; direction <= 1; direction++) {
                int consumed = index >= 0 && x < markers[index][0];
                int crossed = direction < 0 && x < left_edge;
                reset();
                set_x((s16)x);
                body.phase_flags = 0x840;
                body.field_e0 = (s16)index;
                body.motion_x = direction;
                count = 65535;
                assert(func_8018675C(&body) == crossed);
                assert(count == (consumed ? 0 : 65535));
                assert(route_value == (consumed ? markers[index][3] : 513));
                assert(body.field_e0 == (consumed ? -1 : index));
                assert(body.phase_flags == (crossed ? 0x848u : 0x840u));
                assert(body.break_count == (crossed ? 0 : 31));
            }
        }
    }
}

static void check_overlapping_fields(void)
{
    round_index = 1;
    left_edge = -18;
    markers[0][0] = 20;
    markers[0][3] = -43;
    reset();
    set_x(-25);
    body.field_e0 = 0;
    body.phase_flags = 0xfff;
    body.motion_x = 8;
    count_address = &body.phase_flags;
    value_address = &body.motion_x;
    assert(func_8018675C(&body) == 1);
    assert(body.phase_flags == 0x1008 && body.motion_x == -43);
    assert(body.field_e0 == -1 && body.break_count == 0);

    reset();
    markers[0][3] = -234;
    count_address = &markers[0][3];
    body.phase_flags = 0x800;
    body.field_e0 = 0;
    set_x(0);
    assert(func_8018675C(&body) == 0);
    assert(markers[0][3] == -233 && route_value == -234);
}

int main(void)
{
    check_walls();
    check_depth();
    check_markers();
    check_overlapping_fields();
    return 0;
}

/* Independent action scripts, actor slots, and distance service for the Tower. */
#include "steam_knight_action_search.h"
#include <assert.h>
#include <string.h>

s16 func_80185E80(SteamKnightPart *body);

static const u32 mode_addr = 0x801e4bbcu, tables = 0x801959e0u;
static const u32 game_flags = 0x801e43a0u, marker = 0x801e43b4u;
static const u32 marker_tags = 0x80195af8u, player_z = 0x80126cb8u;
static const u32 slots = 0x801e4a58u, first_table = 0x80004000u;
static const u32 second_table = 0xa00a8000u;
static const u32 slot_id = 0x80020000u;
static SteamKnightPart body, part;
static s32 distance;
static unsigned distance_queries;
static int change_choice;
static struct { u32 key, value; } cells[40];
static unsigned cell_count;

static void write_cell(u32 address, u32 value)
{
    unsigned i;
    for (i = 0; i < cell_count; ++i)
        if (cells[i].key == address) {
            cells[i].value = value;
            return;
        }
    assert(cell_count < sizeof cells / sizeof cells[0]);
    cells[cell_count].key = address;
    cells[cell_count++].value = value;
}

void *bfm_tower_spawn_pointer(u32 address)
{
    unsigned i;
    if (address == slot_id || address == (slot_id | 0x20000000u))
        return &part;
    for (i = 0; i < cell_count; ++i)
        if (cells[i].key == address) return &cells[i].value;
    assert(0 && "unmapped fixture address");
    return 0;
}

static void script_entry(u32 table, int index, u16 delay, u16 action)
{
    u32 address = table + 4u * (u32)index;
    write_cell(address, delay);
    write_cell(address + 2, action);
}

static void set_index(SteamKnightPart *actor, s16 index)
{
    memcpy(actor->pad102, &index, sizeof index);
}

static void reset(void)
{
    unsigned i;
    cell_count = distance_queries = 0;
    change_choice = 0;
    distance = 25600;
    memset(&body, 0, sizeof body);
    memset(&part, 0, sizeof part);
    body.field_e0 = -42;
    body.hp = 128;
    write_cell(mode_addr, 0);
    write_cell(tables, first_table);
    write_cell(tables + 4, second_table);
    write_cell(game_flags, 0);
    write_cell(player_z, 0);
    write_cell(marker, 0);
    write_cell(marker_tags, 0);
    for (i = 0; i < 12; ++i) write_cell(slots + 4 * i, 0);
}

s32 func_80132E6C(SteamKnightPart *actor)
{
    assert(actor == &body);
    ++distance_queries;
    if (change_choice) {
        set_index(actor, 7);
        actor->phase_flags = 2;
        write_cell(mode_addr, 1);
        write_cell(player_z, (u16)-1);
        write_cell(game_flags, change_choice == 2 ? 0x2000 : 0);
    }
    return distance;
}

static void slot_searches(void)
{
    static const struct { int delta, forward, beside; } spans[] = {
        {192, 0, 0}, {193, 1, 0}, {447, 1, 0}, {448, 0, 0},
        {-4, 0, 0}, {-3, 0, 1}, {0, 0, 1}, {3, 0, 1}, {4, 0, 0}
    };
    unsigned i, j, side;
    for (i = 0; i < 12; ++i)
        for (j = 0; j < sizeof spans / sizeof spans[0]; ++j)
            for (side = 0; side < 2; ++side) {
                u16 z = side ? 0x8001u : 1u;
                u16 other = (u16)(z - spans[j].delta);
                reset();
                memcpy(&body.field_0e, &z, sizeof z);
                memcpy(&part.field_0e, &other, sizeof other);
                write_cell(slots + 4 * i, slot_id);
                assert(func_8018AC48(&body) == (spans[j].forward ? (int)i : -1));
                assert(func_8018AD20(&body) == (spans[j].beside && !(i & 1) ? (int)i : -1));
                part.field_fc = 0x8000;
                assert(func_8018AC48(&body) == -1 && func_8018AD20(&body) == -1);
            }

    reset();
    write_cell(slots + 4, slot_id);
    write_cell(slots + 16, slot_id | 0x20000000u);
    assert(func_8018AD20(&body) == 4);
    part.field_0e = -200;
    assert(func_8018AC48(&body) == 1);
}

static void marker_and_distance(void)
{
    static const s16 marks[] = {-32768, -7, 0, 12, 32767};
    static const s16 positions[] = {-32768, -1, 0, 32767};
    static const s32 distances[] = {-1, 25599, 25600, 0x7fffffff};
    unsigned i, j, k;
    for (i = 0; i < sizeof marks / sizeof marks[0]; ++i) {
        u32 tag = marker_tags + 8u * (u32)marks[i];
        reset();
        write_cell(marker, (u16)marks[i]);
        write_cell(tag, (u16)-9);
        assert(func_80186144(&body, -9) == marks[i]);
        assert(func_80186144(&body, 0xfff7) == -1);
    }
    for (i = 0; i < sizeof positions / sizeof positions[0]; ++i)
        for (j = 0; j < sizeof positions / sizeof positions[0]; ++j)
            for (k = 0; k < sizeof distances / sizeof distances[0]; ++k) {
                reset();
                body.field_0e = positions[i];
                write_cell(player_z, (u16)positions[j]);
                distance = distances[k];
                assert(func_801860F4(&body, -191) ==
                       (positions[j] <= positions[i] && distances[k] >= 25600));
                assert(distance_queries == 1);
            }
}

/* Compare all actor bytes so the choices that skip advancing retain the script cursor. */
static void expect_choice(s16 returned, s16 index, s16 delay, u16 count, s16 target)
{
    SteamKnightPart expected;
    memcpy(&expected, &body, sizeof expected);
    set_index(&expected, index);
    expected.break_delay = delay;
    expected.break_count = count;
    expected.field_e0 = target;
    assert(func_80185E80(&body) == returned);
    assert(!memcmp(&body, &expected, sizeof body));
}

static void choices(void)
{
    unsigned i, hp, armed;
    static const struct { u16 raw; u32 flags; s16 expected; } basic[] = {
        {0x80fe, 0, 254}, {0, 0, 0}, {1, 0, 1}, {0x8105, 2, 5},
        {5, 8, 0}, {5, 0x400, 0}, {5, 0x800, 5},
        {8, 0x800, 0}, {0x108, 2, 0}, {0x108, 10, 0}
    };
    for (i = 0; i < sizeof basic / sizeof basic[0]; ++i) {
        int deferred = i >= 8;
        int increments = i == 3 || i == 8;
        reset();
        script_entry(first_table, 0, 5, basic[i].raw);
        script_entry(first_table, 1, (u16)-3, 0);
        body.phase_flags = basic[i].flags;
        body.break_count = 0xffff;
        expect_choice(basic[i].expected, deferred ? 0 : 1,
                      deferred ? 0 : -3, increments ? 0 : 0xffff, -42);
    }
    for (i = 6; i <= 7; ++i)
        for (hp = 0; hp < 2; ++hp)
            for (armed = 0; armed < 2; ++armed) {
                reset();
                script_entry(first_table, 0, 0, (u16)i);
                script_entry(first_table, 1, 15, 0);
                body.hp = hp ? 129 : 128;
                body.phase_flags = armed ? 0x800u : 0;
                expect_choice(armed || hp ? (s16)i : 0, 1, 15, 0, -42);
            }
    for (i = 0; i < 2; ++i) {
        reset();
        body.phase_flags = i ? 0x200u : 0u;
        body.break_delay = i ? 0 : -2;
        set_index(&body, -32768);
        expect_choice(0, -32768, body.break_delay, 0, -42);
    }
    reset();
    write_cell(mode_addr, 0xffff);
    write_cell(tables + 4u * 0xffff, second_table);
    set_index(&body, 32767);
    script_entry(second_table, 32767, 19, 0x1ef);
    script_entry(second_table, -32768, 75, 0);
    expect_choice(239, -32768, 75, 0, -42);
}

static void root_searches(void)
{
    static const struct { u16 action; int match; s16 chosen, target; } cases[] = {
        {3, 0, 0, -1}, {3, 1, 3, 10}, {4, 0, 1, -1}, {4, 1, 4, 7},
        {8, 0, 6, -1}, {8, 1, 8, 11}, {9, 0, 1, -1}, {9, 1, 9, 11}
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        reset();
        script_entry(first_table, 0, 0, cases[i].action);
        script_entry(first_table, 1, 4, 0);
        if (cases[i].action == 8) body.phase_flags = 0x400;
        if (cases[i].match) {
            write_cell(slots + 40, slot_id);
            if (cases[i].action == 4) {
                part.field_0e = -311;
                write_cell(slots + 28, slot_id | 0x20000000u);
            }
            write_cell(marker, 11);
            write_cell(marker_tags + 8 * 11, cases[i].action);
        }
        expect_choice(cases[i].chosen, 1, 4, 0, cases[i].target);
    }
}

static void callback_changes(void)
{
    unsigned i;
    for (i = 0; i < 4; ++i) {
        SteamKnightPart expected;
        int sentinel = i & 1;
        reset();
        script_entry(first_table, 0, 11, 0x102);
        script_entry(first_table, 1, 13, 0);
        script_entry(first_table, 8, sentinel ? 0xffffu : 53u, 0);
        script_entry(second_table, 0, (u16)-7, 0);
        script_entry(second_table, 8, 101, 0);
        write_cell(player_z, 1);
        change_choice = i & 2 ? 2 : 1;
        memcpy(&expected, &body, sizeof expected);
        set_index(&expected, sentinel ? 0 : 8);
        expected.phase_flags = 2;
        expected.break_delay = sentinel ? -7 : 53;
        expected.break_count = 1;
        assert(func_80185E80(&body) == (i & 2 ? 0 : 2));
        assert(distance_queries == 1 && !memcmp(&expected, &body, sizeof body));
    }
}

int main(void)
{
    slot_searches();
    marker_and_distance();
    choices();
    root_searches();
    callback_changes();
    return 0;
}

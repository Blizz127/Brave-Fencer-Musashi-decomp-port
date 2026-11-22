#include "forest_kind2_phase.h"
#include <assert.h>
#include <string.h>

static ForestPlayerState actor;
static s32 stop, mode, ground, sample_distance, distance, live_y, fixed_y, low_byte;
static const u32 actor_id = 0x13579bdu, handler_id = 0x2468aceu;
u32 D_8011F9C4;

enum Event { ENTRY, MODE, ADDRESS, POST, LOOKUP, CALL, GROUND, RESET, ANIMATE,
             COUNT, SET_KIND, ACTION, DISTANCE, GATE, FLAG, VECTOR, VALUE, KEEP };
static enum Event events[24];
static unsigned count;
static void record(enum Event event)
{
    assert(count < sizeof(events) / sizeof(events[0]));
    events[count++] = event;
}
#define EXPECT(...) do { \
    const enum Event expected[] = { __VA_ARGS__ }; \
    assert(sizeof(expected) == count * sizeof(events[0])); \
    assert(!memcmp(expected, events, sizeof(expected))); \
} while (0)

/* These predicates and effect handlers belong to other paths. */
#define FALSE_PREDICATE(name) s32 name(ForestPlayerState *a) { assert(a == &actor); return 0; }
#define UNEXPECTED(name) void name(ForestPlayerState *a) { (void)a; assert(0); }
FALSE_PREDICATE(func_80149AA8)
FALSE_PREDICATE(func_80149B54)
FALSE_PREDICATE(func_80149A64)
FALSE_PREDICATE(func_80149CD4)
FALSE_PREDICATE(func_80149744)
UNEXPECTED(func_80149AD4)
UNEXPECTED(func_80149BAC)
UNEXPECTED(func_800CCCC0)
UNEXPECTED(func_8015E880)
UNEXPECTED(func_80149788)

s32 func_80161B18(ForestPlayerState *a) { assert(a == &actor); record(ENTRY); return stop; }
s32 func_801619A4(ForestPlayerState *a) { assert(a == &actor); record(MODE); return mode; }
u32 bfm_forest_action_actor_address(ForestPlayerState *a) { assert(a == &actor); record(ADDRESS); return actor_id; }
s32 func_801498E0(u32 id) { assert(id == actor_id); record(POST); return -1; }
s32 func_80149954(ForestPlayerState *a) { (void)a; assert(0); return 0; }
s32 func_801496D4(ForestPlayerState *a) { assert(a == &actor); return ground; }
s32 func_80149704(ForestPlayerState *a) { assert(a == &actor); record(GROUND); return 0; }
s32 func_80148AFC(ForestPlayerState *a) { assert(a == &actor); return low_byte; }
s32 func_80146D90(ForestPlayerState *a) { assert(a == &actor); record(RESET); return 0; }
s32 func_801488A8(ForestPlayerState *a) { assert(a == &actor); return sample_distance; }
s32 func_80161208(ForestPlayerState *a) { assert(a == &actor); record(KEEP); return 1; }
s32 func_801725A4(ForestPlayerState *a) { (void)a; assert(0); return 0; }

u32 bfm_forest_player_table_word(u32 table, u32 index)
{
    assert(table == 0x801891b8u && index == 0x8002u);
    record(LOOKUP); return handler_id;
}
void bfm_forest_player_call(u32 target, ForestPlayerState *a)
{
    assert(target == handler_id && a == &actor); record(CALL);
    if (sample_distance) a->field_24.word = a->field_2c.word = 0;
}
void func_80147078(ForestPlayerState *a, u16 kind)
{
    assert(a == &actor && kind == (sample_distance ? 1 : 0)); record(SET_KIND);
}
s32 func_8015A264(ForestPlayerState *a) { assert(a == &actor && sample_distance); record(ACTION); return 0; }
s32 func_80159B70(ForestPlayerState *a) { assert(a == &actor && !sample_distance); record(ACTION); return 0; }
s32 bfm_forest_kind2_distance(const s16 first[3], const s16 last[3])
{
    assert(first[0] == 0 && first[1] == 0 && first[2] == 0);
    assert(last[0] == -32 && last[1] == 0 && last[2] == 15);
    record(DISTANCE); actor.field_2c.word = live_y; return distance;
}
s32 func_80154150(ForestPlayerState *a, s32 index)
{
    assert(a == &actor && index == (ground ? 15 : 9)); record(ANIMATE); return 0;
}
s32 func_801541D8(ForestPlayerState *a, s32 index, s32 frames)
{
    assert(a == &actor && index == 13 && frames == 6); record(COUNT); return 0;
}
u32 func_8016F1AC(void) { record(GATE); actor.field_2c.word = fixed_y; return 0; }
s32 func_80029178(u32 index) { assert(index == 33); record(FLAG); return low_byte; }
s32 bfm_forest_action_vector(ForestActionPlayer *a, u32 vector)
{
    assert(a == &actor && vector == 0x8018914cu); record(VECTOR); a->field_16e = 0xf123; return 0;
}
void func_80161CD0(ForestPlayerState *a, u32 value) { assert(a == &actor && value == 0xf123u); record(VALUE); }

static void reset(void)
{
    memset(&actor, 0, sizeof(actor)); actor.kind = 0x8002; actor.field_23c = 0x71f424b0u;
    stop = mode = ground = sample_distance = distance = live_y = fixed_y = low_byte = 0;
    D_8011F9C4 = 0; count = 0;
}
static void run(void)
{
    func_8015B950(&actor); assert(actor.field_23c == 0x71f424b0u);
}
static void check_distance(s32 result, s32 current, s32 expected)
{
    reset(); sample_distance = 1; distance = result; live_y = current;
    actor.field_24.word = -32 * 65536 + 19; actor.field_2c.word = 15 * 65536 + 7;
    run(); EXPECT(ENTRY, MODE, LOOKUP, CALL, SET_KIND, ACTION, DISTANCE);
    assert(actor.field_2c.word == expected);
}
int main(void)
{
    reset(); stop = 0x10000; run(); EXPECT(ENTRY);
    reset(); mode = 3; run(); EXPECT(ENTRY, MODE, ADDRESS, POST, GATE, KEEP);
    reset(); ground = 1; low_byte = 0x100; run();
    EXPECT(ENTRY, MODE, LOOKUP, CALL, GROUND, RESET, COUNT); assert(actor.counter == 1);
    reset(); ground = 1; low_byte = 1; actor.field_24.word = 65536; run();
    EXPECT(ENTRY, MODE, LOOKUP, CALL, GROUND, ANIMATE); assert(actor.counter == 1);
    check_distance(2, -65536, -131072); check_distance(2, -131073, -131073);
    check_distance(0xffff, 100000, 65536); check_distance(0x10000, 17, 0);
    reset(); actor.animation_flags = 0x8000; run(); EXPECT(ENTRY, MODE, LOOKUP, CALL, SET_KIND, ACTION);
    reset(); actor.field_2c.word = -11 * 65536; fixed_y = 1; run();
    EXPECT(ENTRY, MODE, GATE, LOOKUP, CALL, SET_KIND, ACTION, ANIMATE, VECTOR, VALUE);
    reset(); actor.animation_flags = 0x8001; D_8011F9C4 = 65536; low_byte = 0x100; run();
    EXPECT(ENTRY, MODE, GATE, FLAG, LOOKUP, CALL, SET_KIND, ACTION, ANIMATE, VECTOR, VALUE);
    reset(); D_8011F9C4 = 65536; low_byte = 1; run(); EXPECT(ENTRY, MODE, GATE, FLAG, KEEP);
    return 0;
}

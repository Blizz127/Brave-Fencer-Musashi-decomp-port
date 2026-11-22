/* Hosted action dispatch using synthetic table entries and observed service boundaries. */
#include "forest_player_state.h"
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

_Static_assert(offsetof(ForestPlayerState, action) == 2 &&
               offsetof(ForestPlayerState, field_a8) == 0xa8, "guest actor offsets");

enum Step {
    TABLE, TARGET, REFRESH, CONTACT_ONE, CONTACT_TWO, RESET,
    LAND, DEFLECT, PREPARE, IMPULSE, RESOLVE, SET_MODE, FINISH, SYNTHETIC, END
};
enum Entry { DISPATCH, ACTION_ZERO, COLLECT, CHOOSE, AVAILABLE, FLAGS, ADVANCE };
enum Target { KNOWN, ZERO, UNKNOWN };
/* Both tokens belong to this fixture. Only the zero-action entry calls real game C. */
static const u32 zero_action = 0xaa001010u, alternate_action = 0xaa002020u;
static const u32 action_table = 0x80189310u;
static const s32 return_values[] = {0, -123456789, 0x10000, INT_MIN, 0x5432107};

typedef struct {
    enum Entry entry;
    u16 action;
    s32 flags, first_contact, second_contact, service_result;
    u8 before_clamp;
    u32 service_word;
    int wrong_table;
    enum Target target;
    enum Step unavailable;
} Case;

ForestPlayerState D_80126B58;
u32 D_8011F730;
static ForestPlayerState other, expected, *active;
static u32 expected_word, side_effects, actor_address;
static const enum Step *trace;
static unsigned position, runs;
static Case scenario;
static jmp_buf declined;

static void visit(enum Step step, ForestPlayerState *actor)
{
    assert(trace[position++] == step && actor == active);
    assert(memcmp(actor, &expected, sizeof(expected)) == 0);
    assert(D_8011F730 == expected_word);
    if (step == scenario.unavailable) longjmp(declined, 1);
    ++side_effects;
}

u32 bfm_forest_player_table_word(u32 address, u32 index)
{
    visit(TABLE, active);
    assert(address == action_table && index == scenario.action);
    /* This owner has a word only if its table identity and unsigned index agree. */
    if (address != (scenario.wrong_table ? action_table + 4u : action_table))
        longjmp(declined, 1);
    active->action = expected.action = (u16)(index ^ 0x8001u);
    if (scenario.target == ZERO) return 0;
    if (scenario.target == UNKNOWN) return 0xaa001014u;
    return index == 0 ? zero_action : alternate_action;
}

s32 bfm_forest_player_call_result(u32 address, ForestPlayerState *actor)
{
    visit(TARGET, actor);
    if (address == zero_action) {
        assert(scenario.action == 0);
        return func_8015AD78(actor);
    } else if (address == alternate_action) {
        assert(scenario.action != 0);
        visit(SYNTHETIC, actor);
        actor->phase = expected.phase = 37;
        return scenario.service_result;
    } else {
        assert((address == 0 && scenario.target == ZERO) ||
               (address == 0xaa001014u && scenario.target == UNKNOWN));
        longjmp(declined, 1);
    }
}

/* The scratch-stack use inside this service is opaque to the hosted caller. */
s32 func_8014E934(ForestPlayerState *actor)
{
    visit(REFRESH, actor);
    actor->flags = expected.flags = (u32)scenario.first_contact;
    return 0x4000; /* The refresh's return does not participate in the OR. */
}
s32 func_8014CC28(ForestPlayerState *actor)
{
    visit(CONTACT_ONE, actor);
    s32 result = (s32)actor->flags;
    actor->flags = expected.flags = (u32)scenario.second_contact;
    return result;
}
s32 func_8014F3E8(ForestPlayerState *actor)
{
    visit(CONTACT_TWO, actor);
    s32 result = (s32)actor->flags;
    actor->flags = expected.flags = 0x57a1c000u;
    actor->field_a8 = expected.field_a8 = scenario.before_clamp;
    return result;
}
void func_801599A4(ForestPlayerState *actor)
{
    visit(RESET, actor);
    actor->state = expected.state = 45;
}
void func_8015BDD0(ForestPlayerState *actor) { visit(LAND, actor); }
void func_8015BF48(ForestPlayerState *actor) { visit(DEFLECT, actor); }

void func_80161124(ForestPlayerState *actor)
{
    /* The game clamps the current field before this first state service. */
    expected.field_a8 = scenario.before_clamp > 24 ? 24 : scenario.before_clamp;
    visit(PREPARE, actor);
    actor->field_a8 = expected.field_a8 = 211;
}
void func_801477E8(ForestPlayerState *actor, s32 amount)
{
    visit(IMPULSE, actor);
    assert(amount == -0x120000);
    actor->mode = expected.mode = 79;
}
u32 bfm_forest_action_actor_address(ForestPlayerState *actor)
{
    visit(RESOLVE, actor);
    assert((uintptr_t)actor != actor_address && (u32)(uintptr_t)actor != actor_address);
    actor->phase = expected.phase = 31;
    return actor_address;
}
s32 func_8014C010(s32 address, s32 mode)
{
    visit(SET_MODE, active);
    assert((u32)address == actor_address && mode == 2 && active->phase == 31);
    D_8011F730 = expected_word = scenario.service_word;
    return -791; /* The caller takes its return from FINISH. */
}
s32 func_80146CA0(ForestPlayerState *actor)
{
    expected_word |= 1; /* The game sets this bit after SET_MODE returns. */
    visit(FINISH, actor);
    actor->kind = expected.kind = 0xbeef;
    return scenario.service_result;
}

static s32 invoke(void)
{
    switch (scenario.entry) {
    case DISPATCH: return func_8015AD3C(active);
    case ACTION_ZERO: return func_8015AD78(active);
    case COLLECT: return func_801612B8(active, scenario.flags);
    case CHOOSE: return func_8016130C(active, scenario.first_contact);
    case AVAILABLE: return func_80161104(active);
    case FLAGS: return func_80161208(active);
    case ADVANCE: return func_8015ADB0(active);
    }
    assert(0);
    return 0;
}

static void exercise(Case plan, const enum Step *steps, s32 result, int must_decline)
{
    const int separate = runs++ & 1;
    scenario = plan;
    trace = steps;
    position = 0;
    memset(&D_80126B58, 0x55, sizeof(D_80126B58));
    memset(&other, 0x77, sizeof(other));
    active = separate ? &other : &D_80126B58;
    /* Ordinary guest RAM words depend on the actor being resolved. */
    actor_address = separate ? 0x80198740u : 0x80146d00u;
    if (actor_address == (u32)(uintptr_t)active) actor_address += 0x1000u;
    active->action = plan.action;
    active->field_a8 = plan.entry == ADVANCE ? plan.before_clamp : 117;
    active->flags = (u32)plan.first_contact;
    if (steps[0] == REFRESH || plan.entry == DISPATCH || plan.entry == ACTION_ZERO)
        active->flags ^= 0x1000u;
    D_8011F730 = expected_word = 0x12345678u;
    side_effects = 18;
    memcpy(&expected, active, sizeof(expected));
    ForestPlayerState before_global, before_other;
    memcpy(&before_global, &D_80126B58, sizeof(before_global));
    memcpy(&before_other, &other, sizeof(before_other));
    const u32 before_word = D_8011F730, before_effects = side_effects;
    if (!setjmp(declined)) {
        s32 actual = invoke();
        assert(!must_decline && actual == result && trace[position] == END);
        assert(memcmp(active, &expected, sizeof(expected)) == 0);
        assert(D_8011F730 == expected_word && side_effects == before_effects + position);
    } else {
        assert(must_decline && trace[position] == END);
        if (steps[position - 1] == TARGET)
            assert(active->action == (u16)(plan.action ^ 0x8001u));
        if (steps[position - 1] == RESOLVE)
            assert(active->mode == 79 && active->field_a8 == 211 && side_effects > before_effects);
        if (steps[position - 1] == FINISH) {
            assert(active->field_a8 == 211 && active->mode == 79);
            assert(D_8011F730 == (plan.service_word | 1u) && side_effects > before_effects);
        }
        /* The caller owns rollback, including previously completed service effects. */
        memcpy(&D_80126B58, &before_global, sizeof(before_global));
        memcpy(&other, &before_other, sizeof(before_other));
        D_8011F730 = before_word;
        side_effects = before_effects;
        assert(memcmp(&D_80126B58, &before_global, sizeof(before_global)) == 0);
        assert(memcmp(&other, &before_other, sizeof(before_other)) == 0);
        assert(D_8011F730 == before_word && side_effects == before_effects);
    }
    assert(separate ? memcmp(&D_80126B58, &before_global, sizeof(before_global)) == 0 :
                      memcmp(&other, &before_other, sizeof(before_other)) == 0);
}

int main(void)
{
    static const enum Step slot[] = {TABLE, TARGET, SYNTHETIC, END};
    static const enum Step land[] = {RESET, LAND, END}, deflect[] = {RESET, DEFLECT, END};
    static const enum Step none[] = {END};
    static const enum Step advance[] = {PREPARE, IMPULSE, RESOLVE, SET_MODE, FINISH, END};
    static const enum Step through[] = {
        TABLE, TARGET, REFRESH, CONTACT_ONE, CONTACT_TWO, PREPARE, IMPULSE, RESOLVE, SET_MODE, FINISH, END
    };
    static const enum Step landing[] = {REFRESH, CONTACT_ONE, CONTACT_TWO, RESET, LAND, END};
    static const enum Step deflection[] = {REFRESH, CONTACT_ONE, CONTACT_TWO, RESET, DEFLECT, END};
    static const enum Step collect[] = {REFRESH, CONTACT_ONE, CONTACT_TWO, END};
    Case c = {.unavailable=END, .entry=DISPATCH};
    /* Unsigned action 0 follows the real conditional chain below. */
    for (u32 index = 1; index <= 0xffff; ++index) {
        c.action = (u16)index;
        c.service_result = return_values[index % (sizeof(return_values) / sizeof(return_values[0]))];
        exercise(c, slot, c.service_result, 0);
    }
    /* Check equality with 0x8000 separately from the independent bit-14 test. */
    const struct { s32 contact; const enum Step *steps; s32 result; } choices[] = {
        {0,land,1}, {0x8000,land,1}, {0x4000,deflect,1}, {0xc000,deflect,1},
        {0x14001,deflect,1}, {-1,deflect,1}, {0x8001,none,0},
        {0x18000,none,0}, {INT_MIN,none,0}, {1,none,0}
    };
    c.entry = CHOOSE;
    for (unsigned i=0; i<sizeof(choices)/sizeof(choices[0]); ++i) {
        c.first_contact = choices[i].contact;
        exercise(c, choices[i].steps, choices[i].result, 0);
    }
    c.entry = COLLECT;
    c.first_contact = INT_MIN | 4; c.second_contact = 0x8000;
    for (unsigned i=0; i<4; ++i) {
        const s32 flags[] = {0,1,INT_MIN,INT_MIN+1};
        c.flags = flags[i];
        exercise(c, collect + (c.flags & 1), INT_MIN | 0x8004, 0);
    }
    c.flags = 0; c.first_contact = INT_MIN | 0x4004; c.second_contact = 0xc004;
    exercise(c, collect, INT_MIN | 0xc004, 0);
    c.first_contact = 0; c.second_contact = 0x8000;
    c.entry = FLAGS; exercise(c, landing, 1, 0);
    c.entry = AVAILABLE; exercise(c, landing, 0, 0);
    c.entry = ACTION_ZERO; exercise(c, landing, 0, 0);
    c.entry = DISPATCH; c.action = 0; c.service_result = INT_MIN;
    exercise(c, (const enum Step[]){TABLE,TARGET,REFRESH,CONTACT_ONE,CONTACT_TWO,RESET,LAND,END}, 0, 0);
    c.entry = ACTION_ZERO;
    c.first_contact = 0x8000; c.second_contact = 0x4000;
    exercise(c, deflection, 0, 0);
    c.first_contact = 1; c.second_contact = 0x8000;
    c.entry = FLAGS; exercise(c, collect, 0, 0);
    c.entry = AVAILABLE; exercise(c, collect, 1, 0);

    const u8 fields[] = {0,23,24,25,255};
    for (unsigned i=0; i<sizeof(fields)/sizeof(fields[0]); ++i) {
        c.action = 0; c.before_clamp = fields[i]; c.service_word = 0x8ac00010u | (i & 1u);
        c.service_result = return_values[i];
        c.entry = ADVANCE; exercise(c, advance, c.service_result, 0);
        c.entry = ACTION_ZERO; exercise(c, through + 2, c.service_result, 0);
        c.entry = DISPATCH; exercise(c, through, c.service_result, 0);
    }
    c.wrong_table = 1;
    exercise(c, (const enum Step[]){TABLE, END}, 0, 1);
    c.wrong_table = 0;
    c.action = 0xffff;
    for (c.target=ZERO; c.target<=UNKNOWN; ++c.target)
        exercise(c, (const enum Step[]){TABLE,TARGET,END}, 0, 1);
    c.target = KNOWN; c.action = 0;
    for (unsigned i=0; through[i]!=END; ++i) {
        enum Step prefix[sizeof(through)/sizeof(through[0])];
        memcpy(prefix, through, sizeof(through)); prefix[i+1] = END;
        c.unavailable = through[i];
        exercise(c, prefix, 0, 1);
    }
    return 0;
}

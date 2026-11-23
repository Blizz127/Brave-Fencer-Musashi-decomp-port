#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include "tomb_opening.h"

CemeteryPlayer D_80126B58;
CemeteryActor *volatile D_801270C8;
u8 D_801932E8[1];

static CemeteryActor slab, avatar, guide, replacement, key, effect;
static unsigned sequence, sound_count, effects, flag_count, samples;
static int checking_setup, provide_effect, change_guide, change_effect, distance, simultaneous;
static u32 script;
static int busy_queries, distance_queries, key_scripts;

void func_8017D6C0(void)
{
    assert(checking_setup && sequence++ == 0);
    assert(slab.interaction == 0x29 && slab.phase == 1 && slab.remaining == 13);
    if (change_guide)
        D_801270C8 = &replacement;
}

void func_8012A828(CemeteryActor *actor, const void *animation)
{
    assert(checking_setup && sequence++ == 1);
    assert(actor == (change_guide ? &replacement : &guide) && animation == D_801932E8);
    assert(slab.phase == 2 && slab.remaining == 40);
    assert(actor->phase == 4 && actor->remaining == 20);
}

CemeteryActor *func_8012C658(int kind, int variant, CemeteryActor *parent)
{
    assert(checking_setup && sequence++ == 2);
    assert(kind == 136 && variant == 0 && parent == &slab);
    return &key;
}

void func_8002D4C8(u16 command, u16 argument)
{
    assert(command == 1321 && argument == 0);
    if (checking_setup) {
        assert(sequence++ == 3 && slab.sound_ticks == 9);
    } else {
        assert(slab.sound_ticks >= 16);
        if (simultaneous)
            assert(slab.remaining == 1 && slab.phase == 2 && flag_count == 0 && sequence == 0);
        ++sound_count;
    }
}

void func_8017D6E8(void)
{
    assert(!checking_setup && sequence++ == 0);
    assert(slab.phase == 3 && slab.remaining == 0 && flag_count == 0);
    if (simultaneous)
        assert(sound_count == 1 && slab.sound_ticks == 0 && slab.x == 11 && slab.z == 31);
}

u32 func_80029124(u32 bit, u32 set)
{
    assert(!checking_setup && sequence++ == 1);
    assert(bit == 143 && set == 1 && slab.phase == 3 && slab.remaining == 0);
    if (simultaneous)
        assert(slab.x == 11 && slab.z == 31);
    ++flag_count;
    return 0;
}

CemeteryActor *func_80143B6C(CemeteryActor *parent, int variant)
{
    assert(parent == &slab && variant == 1);
    if (parent->remaining == 0)
        assert(sequence == 2 && flag_count == 1);
    ++effects;
    return provide_effect ? &effect : NULL;
}

int func_8005C49C(void)
{
    ++samples;
    if (samples & 1) {
        assert(effect.x == (u16)(10 - 32));
        if (change_effect)
            effect.x = 61;
        return -1;
    }
    assert(effect.x == (change_effect ? 124 : (u16)(10 - 32 + 63)));
    if (change_effect)
        effect.z = 23;
    return 63;
}

u32 func_8016F1AC(void)
{
    ++busy_queries;
    return script;
}

int func_8012BD14(CemeteryActor *actor)
{
    assert(actor == &key && busy_queries == 1);
    ++distance_queries;
    return distance;
}

void func_8017D884(void)
{
    assert(busy_queries == 1 && distance_queries == 1 && key.phase == 1);
    ++key_scripts;
}

static void setup_case(int linked, int heading, int selected, int replace, int flagged)
{
    int opens = linked && selected;
    int prompt = linked && heading == 2048 && !selected;
    checking_setup = 1;
    sequence = 0;
    change_guide = replace;
    D_801270C8 = linked ? &guide : NULL;
    D_80126B58.avatar = linked ? &avatar : NULL;
    D_80126B58.selected = selected ? &slab : &key;
    avatar.heading = (s16)heading;
    slab.phase = 1;
    slab.remaining = 13;
    slab.sound_ticks = 9;
    slab.interaction = flagged ? 0x1029 : 0x29;
    guide.phase = replacement.phase = 11;
    guide.remaining = replacement.remaining = 17;
    func_8017E414(&slab);
    assert(sequence == (opens ? 4u : 0u));
    assert(slab.interaction == (prompt ? 0x1029 : 0x29));
    assert(slab.phase == (opens ? 2 : 1) && slab.remaining == (opens ? 40u : 13u));
    assert(slab.sound_ticks == (opens ? 0 : 9));
    assert(guide.phase == (opens && !replace ? 4 : 11));
    assert(guide.remaining == (opens && !replace ? 20u : 17u));
    assert(replacement.phase == (opens && replace ? 4 : 11));
    assert(replacement.remaining == (opens && replace ? 20u : 17u));
    checking_setup = 0;
}

static void motion_test(void)
{
    unsigned tick;
    sequence = sound_count = effects = flag_count = samples = 0;
    provide_effect = 0;
    slab.remaining = 40;
    slab.phase = 2;
    slab.sound_ticks = 0;
    slab.x = 29;
    slab.z = 65510;
    for (tick = 1; tick <= 40; ++tick) {
        func_8017E524(&slab);
        assert(effects == tick && samples == 0);
        assert(slab.remaining == 40 - tick && sound_count == tick / 16);
        assert(slab.sound_ticks == (s16)(tick % 16));
        assert(slab.phase == (tick == 40 ? 3 : 2));
        assert(flag_count == (unsigned)(tick == 40));
        assert(slab.x == (29u ^ ((tick & 1) << 1)));
        assert(slab.z == (u16)(65510 + tick * 2));
    }
}

static void coincident_timers(void)
{
    sequence = sound_count = effects = flag_count = 0;
    slab.remaining = 1;
    slab.phase = 2;
    slab.sound_ticks = 15;
    slab.x = 11;
    slab.z = 31;
    simultaneous = 1;
    func_8017E524(&slab);
    assert(sound_count == 1 && flag_count == 1 && effects == 1);
    assert(slab.x == 9 && slab.z == 33);
    simultaneous = 0;
}

static void effect_case(int mutate)
{
    sequence = sound_count = effects = flag_count = samples = 0;
    provide_effect = 1;
    change_effect = mutate;
    effect.x = 10;
    effect.z = 5;
    slab.sound_ticks = SHRT_MAX;
    slab.remaining = 0;
    func_8017E524(&slab);
    assert(slab.remaining == UINT_MAX && slab.sound_ticks == SHRT_MIN);
    assert(effects == 1 && samples == 2 && flag_count == 0 && sound_count == 0);
    assert(effect.x == (mutate ? 124 : (u16)(10 - 32 + 63)));
    assert(effect.z == (u16)((mutate ? 23 : 5) - 31));
}

static void key_case(u32 busy, int separation)
{
    int eligible = !busy && separation <= 1024;
    busy_queries = distance_queries = key_scripts = 0;
    key.phase = 1;
    script = busy;
    distance = separation;
    func_8017EC30(&key);
    assert(busy_queries == 1 && distance_queries == !busy && key_scripts == eligible);
    assert(key.phase == (eligible ? 2 : 1));
}

int main(void)
{
    int linked, selected, replace, facing, flagged;
    _Static_assert(offsetof(CemeteryActor, phase) == 2, "slab phase");
    _Static_assert(offsetof(CemeteryActor, x) == 6 && offsetof(CemeteryActor, z) == 14, "position");
    _Static_assert(offsetof(CemeteryActor, heading) == 18, "avatar heading");
    _Static_assert(offsetof(CemeteryActor, remaining) == 28, "motion timer");
    _Static_assert(offsetof(CemeteryActor, interaction) == 92, "interaction flags");
    _Static_assert(offsetof(CemeteryActor, sound_ticks) == 132, "sound timer");
    _Static_assert(offsetof(CemeteryPlayer, avatar) == 32, "player actor");
    _Static_assert(offsetof(CemeteryPlayer, selected) == 368, "selected actor");
    for (linked = 0; linked <= 1; ++linked)
        for (selected = 0; selected <= 1; ++selected)
            for (replace = 0; replace <= 1; ++replace)
                for (facing = 2047; facing <= 2049; ++facing)
                    for (flagged = 0; flagged <= 1; ++flagged)
                        setup_case(linked, facing, selected, replace, flagged);
    motion_test();
    coincident_timers();
    effect_case(0);
    effect_case(1);
    key_case(0, INT_MIN);
    key_case(0, 1024);
    key_case(0, 1025);
    key_case(0, INT_MAX);
    key_case(0x80000000u, 1024);
    return 0;
}

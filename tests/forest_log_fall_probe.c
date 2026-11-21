/* Synthetic scene-object witness. Compile with the other scene bodies.
 * Low-address fixtures and -fno-strict-aliasing model the game's overlapping
 * fields. No game data, dispatcher hooks, or console runtime is needed. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../src/overlays/sc01_0077/8017f694.c"

extern void func_8017F290(void);
extern void func_8017F6D8(u8 *actor);
extern void func_8017F460(u8 *actor);
extern void func_8018047C(u8 *actor);
extern void func_80180588(u8 *actor);
extern void func_80180590(u8 *actor);
extern s32 func_8012BEE8(u8 *actor);

s32 D_801270CC[1], D_801270D4, D_8018A8F8;
u16 D_80126B66, D_80126B96;
s16 D_8018A8FC[3] = {1, 8, 16};
s32 D_801D15C4[1], D_801D1CBC[1];
u8 D_8018A8A4[1], D_8018A8B4[1], D_8018A8C4[1], D_80186E48[1];
u8 D_80078EC1, D_8017E1D4[1], D_8017E748[1], D_8017E868[1];
static u8 *last_request;
static int request_count;

void func_8016EDEC(u8 *script, s32 flags)
{
    assert(flags == 0x1000000);
    last_request = script;
    ++request_count;
}

static _Alignas(4) u8 actors[3][0x10c], models[3][0x20];
static u8 *current;
static int predicate_result, predicate_count, change_count;
static int sound_start, sound_bounce, effects, state_zero_calls;
static unsigned bounce_count[3], start_tick[3], settle_tick[3];

static void put16(u8 *p, unsigned offset, u16 v) { memcpy(p + offset, &v, 2); }
static void put32(u8 *p, unsigned offset, u32 v) { memcpy(p + offset, &v, 4); }
static u16 get16(const u8 *p, unsigned offset) { u16 v; memcpy(&v, p + offset, 2); return v; }
static u32 get32(const u8 *p, unsigned offset) { u32 v; memcpy(&v, p + offset, 4); return v; }

static u32 address_of(u8 *p)
{
    uintptr_t a = (uintptr_t)p;
    assert(a <= UINT32_MAX);
    return (u32)a;
}

void *func_8012C1B8(void)
{
    for (unsigned i = 0; i < 3; ++i)
        if (current == actors[i])
            return models[i];
    return NULL;
}

s32 func_8012CAE4(u8 *actor)
{
    (void)actor;
    assert(0 && "the three synthetic model allocations succeeded");
    return 0;
}

void func_8001C214(void *model, s32 *list)
{
    assert(address_of(model) == get32(current, 0x20));
    assert(list == (current[0x70] == 4 ? D_801D1CBC : D_801D15C4));
}

void func_8001D0E8(void *model, s16 width, s16 height)
{
    assert(address_of(model) == get32(current, 0x20));
    assert(width == (current[0x70] == 4 ? 0x7fff : 0x168));
    assert(height == (current[0x70] == 4 ? 0x7fff : 0x140));
}

s32 func_8012A828(u8 *actor, u32 config)
{
    assert(actor == current && config == address_of(D_80186E48));
    return 0;
}

void func_8012B2CC(char *actor)
{
    assert((u8 *)actor == current && get16(current, 2) == 1);
    ++state_zero_calls;
}

void (*D_8018A904[])(u8 *) = {
    func_80180590, func_8017F460, func_8018047C, func_80180588
};

s32 func_8012C51C(void *placement, u8 *owner)
{
    (void)placement;
    (void)owner;
    assert(0 && "physical logs never spawn a child in their fall initializer");
    return 0;
}

s32 func_8012D714(u8 *actor, s32 mode)
{
    assert(actor == current && mode == 2 && get16(actor, 0x34) == 0);
    ++predicate_count;
    if (change_count >= 0)
        put16(actor, 0x34, (u16)change_count);
    return predicate_result;
}

s32 func_8002D4C8(u16 cmd, u16 arg)
{
    assert(arg == 0 && get16(current, 2) == 2);
    if (cmd == 0x495)
        ++sound_start;
    else {
        assert(cmd == 0x487 && get16(current, 0x34) == 0);
        ++sound_bounce;
    }
    return -1;
}

s32 func_8012C588(s16 effect, ForestEffectSource *source)
{
    assert(effect == 0x25 && source == NULL && get16(current, 2) == 3);
    assert((get16(current, 0x5c) & 0xcc00) == 0xcc00);
    ++effects;
    return -1;
}

static void reset(void)
{
    memset(actors, 0, sizeof(actors));
    memset(models, 0, sizeof(models));
    current = actors[0];
    change_count = -1;
    predicate_result = predicate_count = sound_start = sound_bounce = effects = 0;
    D_80126B96 = 0x1234;
    for (unsigned i = 0; i < 3; ++i) {
        put16(actors[i], 2, 1);
        put16(actors[i], 0x70, (u16)(i * 256 + 4));
        put16(actors[i], 0x5c, 0x8000);
        put32(actors[i], 0x20, address_of(models[i]));
    }
}

static void ticks_for_three_roles(void)
{
    reset();
    for (unsigned i = 0; i < 3; ++i) {
        current = actors[i];
        put16(current, 2, 0);
        put16(current, 0x5c, 0);
        put32(current, 0x20, 0);
        func_8017F6D8(current);
        assert(get16(current, 2) == 1 && get16(current, 0x5c) == 0x8000);
    }
    assert(state_zero_calls == 3);
    D_801270CC[0] = D_801270D4 = 0;
    D_80126B66 = 0x5ff;
    D_80078EC1 = 0x80;
    func_8017F290();
    assert(request_count == 0 && D_801270CC[0] == 0);
    D_80126B66 = 0x600;
    func_8017F290();
    assert(request_count == 1 && last_request == D_8017E1D4 && D_801270CC[0] == 1);
    D_80078EC1 = 0x88;
    func_8017F290();
    assert(request_count == 2 && last_request == D_8017E748 && D_801270CC[0] == 2);
    /* One controller update per synthetic loop, followed by all three objects.
     * The renderer and actual VSync cadence are external to these functions. */
    put16(actors[0], 0x5c, 0x8001);
    for (unsigned tick = 1; tick <= 128; ++tick) {
        func_8017F290();
        for (unsigned i = 0; i < 3; ++i) {
            current = actors[i];
            u16 before = get16(current, 2);
            u16 rebounds = get16(current, 0x34);
            func_8017F6D8(current);
            u16 after = get16(current, 2);
            if (before == 1 && after == 2) {
                start_tick[i] = tick;
                assert(get16(models[i], 0x10) == 0 && get32(current, 0xe0) == 0x100000);
                assert(get32(current, 0x1c) == 16 && get16(current, 0x34) == 0);
            } else if (before == 2 && after == 3) {
                settle_tick[i] = tick;
                assert(get16(models[i], 0x10) == 0x400);
            } else if (before == 2 && get16(current, 0x34) > rebounds) {
                ++bounce_count[i];
                assert(get32(current, 0xdc) == 0x4000000);
            }
        }
    }
    for (unsigned i = 0; i < 3; ++i) {
        assert(start_tick[i] == (unsigned)D_8018A8FC[i]);
        assert(settle_tick[i] > start_tick[i] && bounce_count[i] == 2);
        assert(settle_tick[i] - start_tick[i] == 51);
        assert(get16(actors[i], 2) == 3 && get32(actors[i], 0x1c) == 16);
        u8 snapshot[sizeof(actors[i])];
        memcpy(snapshot, actors[i], sizeof(snapshot));
        current = actors[i];
        func_8017F6D8(current);
        assert(!memcmp(snapshot, current, sizeof(snapshot)));
    }
    assert(request_count == 3 && last_request == D_8017E868 && D_801270CC[0] == 3);
    D_801270CC[0] = 0;
    func_8017F290();
    assert(request_count == 4 && last_request == D_8017E1D4 && D_801270CC[0] == 2);
    assert(sound_start == 3 && predicate_count == 3 && sound_bounce == 3 && effects == 3);
}

static void threshold_and_aliases(void)
{
    reset();
    put16(current, 2, 2);
    put32(current, 0xe0, 0xffffffffu);
    put32(current, 0xe4, 2);
    put32(current, 0xdc, 0x03fffffeu);
    func_8018047C(current);
    assert(get32(current, 0xdc) == 0x03ffffff && get16(models[0], 0x10) == 0x3ff);
    assert(predicate_count == 0 && sound_bounce == 0);
    put32(current, 0xe4, 0);
    predicate_result = 1;
    change_count = 2;
    func_8018047C(current);
    assert(D_80126B96 == 0x4002 && predicate_count == 1);
    assert(sound_bounce == 0 && effects == 1 && get16(current, 2) == 3);

    /* The pitch write aliases the flags. The OR must consume the new half. */
    put16(current, 0x5c, 0x0031);
    put32(current, 0x20, address_of(current + 0x5c - 0x10));
    func_8017F694(current);
    assert(effects == 2 && get16(current, 0x5c) == 0xcc00);

    reset();
    put16(current, 2, 2);
    put16(current, 0x34, 1);
    put32(current, 0xe0, 0xfffffffdu);
    put32(current, 0xdc, 0x04010000u);
    func_8018047C(current);
    assert(get32(current, 0xe0) == 2 && get16(current, 0x34) == 2);
    assert(predicate_count == 0 && sound_bounce == 0);
    put32(current, 0xdc, 0x80000000u);
    put32(current, 0xe0, 0);
    func_8018047C(current);
    assert(get16(models[0], 0x10) == 0x8000 && effects == 0);

    /* Fall's pitch may alias its counter before the callback decision. */
    reset();
    put16(current, 2, 2);
    put32(current, 0x20, address_of(current + 0x34 - 0x10));
    put32(current, 0xdc, 0x04000000u);
    func_8018047C(current);
    assert(effects == 1 && predicate_count == 0 && sound_bounce == 0);
}

static void other_kinds_and_dispatch(void)
{
    reset();
    current[0x70] = 0;
    put16(current, 2, 2);
    put32(current, 0x1c, 2);
    put32(current, 0xe4, 7);
    func_8017F6D8(current);
    assert(get32(current, 0x1c) == 1 && get32(current, 0xe0) == 0);
    func_8017F6D8(current);
    assert(get32(current, 0x1c) == 0 && get32(current, 0xe0) == 7);
    assert(func_8012BEE8(current) == 1);
    put32(current, 0x1c, 0x80000000u);
    assert(func_8012BEE8(current) == 0 && get32(current, 0x1c) == 0x7fffffffu);
    put16(current, 2, 0);
    func_8017F6D8(current);
    assert(state_zero_calls == 4 && get16(current, 2) == 1);
}

int main(void)
{
    u16 little = 1;
    assert(*(u8 *)&little == 1);
    ticks_for_three_roles();
    threshold_and_aliases();
    other_kinds_and_dispatch();
    return 0;
}

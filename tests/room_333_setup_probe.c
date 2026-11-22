#include "actor_333.h"
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The synthetic services in this test own all of their callback effects. */
typedef struct { u32 before[3]; Actor333 value; u32 after[2]; } ActorRecord;
typedef struct { u32 before[2]; Actor333Model value; u32 after[3]; } ModelRecord;
static ActorRecord actors[3], wanted_actors[3];
static ModelRecord models[3], wanted_models[3];
_Static_assert(sizeof(Actor333) == 0x10c && sizeof(Actor333Model) == 0x84, "room object strides");
_Static_assert(offsetof(Actor333, model_address) == 0x20 && offsetof(Actor333, shape) == 0x58,
               "actor model and shape");
_Static_assert(offsetof(Actor333, contact_flags) == 0x5c && offsetof(Actor333, vector) == 0xe0,
               "actor contact flags and vector");
_Static_assert(offsetof(Actor333Model, fields.flags) == 4 &&
               offsetof(Actor333Model, fields.table) == 0x20 &&
               offsetof(Actor333Model, fields.options) == 0x2c &&
               offsetof(Actor333Model, fields.vector) == 0x80, "model setup fields");

Actor333Part D_801864C8, D_801864D4;
_Static_assert(sizeof(Actor333Part) == 2 && _Generic(D_801864C8.value, s16: 1, default: 0),
               "room descriptor signed halfword");
static s16 wanted_part[2];
const u32 bfm_room_333_shape_address = 0x81256940u;
const u32 bfm_room_333_table_address = 0xa72cc154u;
static const u32 owner_address[3] = {0xb3813874u, 0x840eb4c0u, 0x80800000u};
static const u32 model_address[3] = {0x80000000u, 0x14ef1cb4u, 0xb4478b98u};

enum Edge { COMPLETE, FLAG, ACQUIRE, RELEASE, BIND, MISSING_ATTACHMENT };
enum Binding { UNTOUCHED, EDIT_MODEL, REPLACE, REPLACE_AND_WRAP, DETACH };
static unsigned active, selected, stale, available, modifications, edge_count;
static s32 event_word, release_word, result;
static enum Edge cut, last;
static enum Binding binding;
static jmp_buf stopped;

static void put16(void *base, size_t offset, u16 value)
{
    memcpy((u8 *)base + offset, &value, sizeof value);
}
static void put32(void *base, size_t offset, u32 value)
{
    memcpy((u8 *)base + offset, &value, sizeof value);
}
static u16 get16(const void *base, size_t offset)
{
    u16 v;
    memcpy(&v, (const u8 *)base + offset, sizeof v);
    return v;
}
static u32 get32(const void *base, size_t offset)
{
    u32 v;
    memcpy(&v, (const u8 *)base + offset, sizeof v);
    return v;
}
static void compare_state(void)
{
    assert(memcmp(actors, wanted_actors, sizeof actors) == 0);
    assert(memcmp(models, wanted_models, sizeof models) == 0);
    assert(D_801864C8.value == wanted_part[0] && D_801864D4.value == wanted_part[1]);
}
static void edge(enum Edge next)
{
    last = next;
    ++edge_count;
    compare_state();
    if (cut == next) longjmp(stopped, 1);
}
static Actor333 *actor(void) { return &actors[active].value; }
static Actor333 *want_actor(void) { return &wanted_actors[active].value; }
static unsigned target(void)
{
    return binding == REPLACE || binding == REPLACE_AND_WRAP ? (selected + 1) % 3 : selected;
}

s32 func_80029178(u32 event)
{
    assert(event == 0x11c && edge_count == 0);
    edge(FLAG);
    if (modifications) {
        put32(actor(), 0x20, model_address[stale]);
        put32(want_actor(), 0x20, model_address[stale]);
        put16(actor(), 0xe6, 0xb871);
        put16(want_actor(), 0xe6, 0xb871);
    }
    return event_word;
}

void *func_8012C1B8(void)
{
    assert(edge_count == 1 && last == FLAG && (u8)event_word == 0);
    edge(ACQUIRE);
    if (modifications) {
        put32(actor(), 0x20, model_address[(stale + 1) % 3]);
        put32(want_actor(), 0x20, model_address[(stale + 1) % 3]);
    }
    return available ? &models[selected].value : NULL;
}

u32 bfm_room_333_model_address(Actor333Model *model)
{
    assert(available && model == &models[selected].value && last == ACQUIRE);
    return model_address[selected];
}

s32 func_8012CAE4(u8 *subject)
{
    assert(subject == (u8 *)actor());
    if ((u8)event_word) {
        assert(edge_count == 1 && last == FLAG);
    } else {
        assert(!available && edge_count == 2 && last == ACQUIRE);
        put32(want_actor(), 0x20, 0);
    }
    edge(RELEASE);
    if (modifications) {
        /* Selected stale storage can be affected by this synthetic release. */
        if ((u8)event_word) {
            size_t bytes = binding == EDIT_MODEL ? 56 : sizeof(Actor333Model);
            memset(&models[stale].value, 0, bytes);
            memset(&wanted_models[stale].value, 0, bytes);
        }
        memset(subject, 0, sizeof(Actor333));
        memset(want_actor(), 0, sizeof(Actor333));
    }
    return release_word;
}

void bfm_room_333_bind_model(Actor333Model *model)
{
    assert(edge_count == 2 && last == ACQUIRE && model == &models[selected].value);
    put32(want_actor(), 0x20, model_address[selected]);
    edge(BIND);
    if (binding == UNTOUCHED) return;
    const unsigned index = target();
    const u32 new_address = binding == DETACH ? 0 : model_address[index];
    put32(actor(), 0x20, new_address);
    put32(want_actor(), 0x20, new_address);
    if (binding == REPLACE_AND_WRAP) {
        put16(actor(), 2, 0xffff);
        put16(want_actor(), 2, 0xffff);
    }
    for (unsigned i = 0; i < 3; ++i) {
        const u32 flags = (i == selected ? 0xf19a22d4u : 0x72402d43u) |
                          (modifications ? 0x8000u : 0u);
        const u16 options = (u16)((i == selected ? 0xe157u : 0xa339u) |
                                  (modifications ? 0x80u : 0u));
        put32(&models[i].value, 4, flags);
        put32(&wanted_models[i].value, 4, flags);
        put16(&models[i].value, 0x2c, options);
        put16(&wanted_models[i].value, 0x2c, options);
        put16(&models[i].value, 0x2e, 0x6edb);
        put16(&wanted_models[i].value, 0x2e, 0x6edb);
    }
    D_801864C8.value = wanted_part[0] = 0x6457;
    D_801864D4.value = wanted_part[1] = -19067;
}

Actor333Model *bfm_room_333_attached_model(const Actor333 *subject)
{
    assert(subject == actor() && last == BIND);
    if (subject->model_address == 0) {
        assert(binding == DETACH && cut == MISSING_ATTACHMENT);
        edge(MISSING_ATTACHMENT);
        assert(0 && "detached model adapter should have stopped");
    }
    for (unsigned i = 0; i < 3; ++i)
        if (subject->model_address == model_address[i]) return &models[i].value;
    assert(0 && "unrecognized synthetic model owner");
    return NULL;
}

u32 bfm_room_333_vector_address(const Actor333 *subject)
{
    assert(subject == actor() && last == BIND);
    return owner_address[active] + 0xe0u;
}

static void expect_success(void)
{
    Actor333Model *m = &wanted_models[target()].value;
    put32(m, 4, get32(m, 4) | 0x8000u);
    put32(want_actor(), 0x58, bfm_room_333_shape_address | 0x40000000u);
    put16(want_actor(), 0x5c, 0xc800);
    wanted_part[0] = wanted_part[1] = 0;
    put32(m, 0x20, bfm_room_333_table_address);
    for (unsigned i = 0; i < 3; ++i) put16(want_actor(), 0xe0 + i * 2, 0x80);
    put16(want_actor(), 0xe6, 0xfffe);
    put16(m, 0x2c, (u16)(get16(m, 0x2c) | 0x80u));
    put32(m, 0x80, owner_address[active] + 0xe0u);
    s32 next = (s32)get16(want_actor(), 2) + 1;
    put16(want_actor(), 2, (u16)next);
    assert(result == next);
}

static void setup(u16 phase, unsigned seed)
{
    u8 *ap = (u8 *)actors, *mp = (u8 *)models;
    for (size_t i = 0; i < sizeof actors; ++i) ap[i] = (u8)(i * 31u + seed + phase);
    for (size_t i = 0; i < sizeof models; ++i) mp[i] = (u8)(i * 17u + seed + phase);
    active = seed % 3;
    selected = (seed / 3) % 3;
    stale = (selected + 2) % 3;
    actor()->type = 0x333;
    actor()->phase = phase;
    actor()->model_address = model_address[(stale + 2) % 3];
    memcpy(wanted_actors, actors, sizeof actors);
    memcpy(wanted_models, models, sizeof models);
    D_801864C8.value = wanted_part[0] = -1107;
    D_801864D4.value = wanted_part[1] = 0x4a42;
    last = COMPLETE;
    edge_count = 0;
    result = 39;
}

static void scenario(u16 phase, unsigned seed, enum Edge where)
{
    setup(phase, seed);
    cut = where;
    if (setjmp(stopped) == 0) {
        result = func_80181FA8(actor());
        assert(cut == COMPLETE);
        if ((u8)event_word || !available) {
            assert(last == RELEASE && result == release_word);
        } else {
            assert(last == BIND && binding != DETACH);
            expect_success();
        }
    } else {
        assert(cut != COMPLETE && last == cut && result == 39);
    }
    compare_state();
}

int main(void)
{
    const s32 predicates[] = {0, 1, 2, 0x80, 0x100, 0x101, -1, -256, INT32_MIN, 0xff};
    const u16 phases[] = {0, 1, 0x7fff, 0x8000, 0xfffe, 0xffff};
    const s32 releases[] = {0, 1, 0xff, 0x100, 0x10000, INT32_MAX, INT32_MIN, -3, -1};
    unsigned seed = 0;
    for (unsigned p = 0; p < sizeof predicates / sizeof predicates[0]; ++p)
        for (available = 0; available < 2; ++available)
            for (unsigned k = 0; k < sizeof phases / sizeof phases[0]; ++k)
                for (binding = UNTOUCHED; binding <= DETACH; ++binding)
                    for (modifications = 0; modifications < 2; ++modifications) {
                        event_word = predicates[p];
                        release_word = releases[seed % (sizeof releases / sizeof releases[0])];
                        int fail = (u8)event_word != 0 || !available;
                        scenario(phases[k], seed, FLAG);
                        if (!(u8)event_word) scenario(phases[k], seed, ACQUIRE);
                        scenario(phases[k], seed, fail ? RELEASE : BIND);
                        scenario(phases[k], seed, !fail && binding == DETACH ? MISSING_ATTACHMENT : COMPLETE);
                        ++seed;
                    }
    return 0;
}

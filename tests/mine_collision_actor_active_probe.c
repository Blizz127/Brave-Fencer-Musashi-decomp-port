#include "mine_collision_actor_active.h"
#include <assert.h>
#include <string.h>

enum { VARIANT = 0xfe, POSITION = 0xdc, VELOCITY = 0xe0 };
static const uint32_t HIDDEN = UINT32_C(1) << 31;

typedef struct {
    Obj8014A2E4 actor, parent;
    uint8_t models[2][22];
    SceneSurfaceBox box;
    uint16_t event, time;
    unsigned segments, transforms, lookups;
    int hit_at, refuse_at, rebind, activate;
} World;

/* Packed actor records use little-endian fields independent of host byte order. */
static uint32_t get(const void *record, unsigned offset, unsigned width)
{
    const uint8_t *bytes = record;
    uint32_t result = 0;
    for (unsigned i = 0; i < width; ++i)
        result |= (uint32_t)bytes[offset + i] << (8 * i);
    return result;
}

static void put(void *record, unsigned offset, unsigned width, uint32_t value)
{
    uint8_t *bytes = record;
    for (unsigned i = 0; i < width; ++i)
        bytes[offset + i] = (uint8_t)(value >> (8 * i));
}

static Obj8014A2E4 *parent(void *context, uint32_t key)
{
    World *w = context;
    return key == 11 ? &w->parent : NULL;
}

static uint8_t *model(void *context, uint32_t key)
{
    World *w = context;
    ++w->lookups;
    if (w->rebind && w->lookups == 1)
        w->actor.probe_object_address = 2;
    return key >= 1 && key <= 2 ? w->models[key - 1] : NULL;
}

static int transform(void *context, Obj8014A2E4 *actor)
{
    World *w = context;
    assert(actor == &w->actor);
    ++w->transforms;
    if (w->activate)
        put(actor, VARIANT, 2, 3);
    return 1;
}

static int segment(void *context, Obj8014A2E4 *actor, MineSweepPoint *a,
                   MineSweepPoint *b, int *hit)
{
    const int first_x = -13, last_x = 4;
    /* Negative, odd midpoint sums round down, including near zero. */
    const int x[] = {first_x, -5, -9, -1, last_x};
    World *w = context;
    assert(actor == &w->actor && w->segments < 5);
    assert(a->x == x[w->segments] && b->x == a->x);
    assert(a->y == 32757 && b->y == a->y && a->z == -23 && b->z == 41);
    ++w->segments;
    put(actor, 0x74, 1, 13 + w->segments);
    if ((int)w->segments == w->refuse_at)
        return 0;
    *hit = (int)w->segments == w->hit_at ? -5 : 0;
    return 1;
}

static MineActiveServices setup(World *w)
{
    memset(w, 0, sizeof(*w));
    w->actor.probe_object_address = w->parent.probe_object_address = 1;
    put(&w->actor, 0x64, 4, 11);
    w->box = (SceneSurfaceBox){.bounds = {-13, 4, -32763, 9, -23, 41}};
    return (MineActiveServices){.context = w, .box = &w->box, .event_code = &w->event,
        .event_time = &w->time, .parent = parent, .model = model,
        .transform = transform, .segment = segment};
}

static void test_motion(void)
{
    const struct { uint32_t angle, fling, damped; } cases[] = {
        {0, 1, 0}, {0, UINT32_MAX, 0}, {0, 0x80000000u, 0x80008000u},
        {0, 0x7fffffffu, 0x7fff7fffu}, {0x7fffff00u, 0x40000, 0x18000},
        {0x80000010u, 0xfffc0000u, 0xfffe8000u}, {2u << 16, 0, 0},
        {0xfffe0000u, 0, 0}, {3u << 16, 0, 0xfffe8000u}, {0xfffd0000u, 0, 0x18000},
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
        World w;
        MineActiveServices s = setup(&w);
        put(&w.actor, POSITION, 4, cases[i].angle);
        put(&w.actor, VELOCITY, 4, cases[i].fling);
        assert(mine_actor_motion(&w.actor, &s));
        uint32_t next = cases[i].angle + cases[i].damped;
        assert(get(&w.actor, VELOCITY, 4) == cases[i].damped);
        assert(get(&w.actor, POSITION, 4) == next && get(w.models[0], 20, 2) == next >> 16);
    }
}

static void test_parent_distance(void)
{
    const struct { int x, player, far; } cases[] = {
        {0,0,0}, {2048,0,0}, {-2048,0,0}, {2049,0,1}, {-2049,0,1},
        {-32768,0,0}, {32767,-32768,0},
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
        World w;
        MineActiveServices s = setup(&w);
        s.player_x = (int16_t)cases[i].player;
        put(&w.actor, 6, 2, (uint16_t)cases[i].x);
        w.rebind = !cases[i].far;
        put(w.models[0], 4, 4, 19);
        put(w.models[1], 4, 4, HIDDEN | 71);
        assert(mine_actor_active(&w.actor, &s));
        assert(w.lookups == (cases[i].far ? 1u : 2u));
        assert(w.transforms == (cases[i].far ? 0u : 1u));
        assert(get(w.models[0], 4, 4) == (cases[i].far ? HIDDEN | 19 : 19));
        assert(get(w.models[1], 4, 4) == (cases[i].far ? HIDDEN | 71 : 71));
    }
}

static void test_sweep(void)
{
    for (int contact = 0; contact <= 5; ++contact) {
        World w;
        MineActiveServices s = setup(&w);
        int hit = -1;
        w.hit_at = contact;
        assert(mine_actor_sweep(&w.actor, &s, &hit));
        assert(hit == !!contact && w.segments == (contact ? (unsigned)contact : 5u));
    }
    World w;
    MineActiveServices s = setup(&w);
    int hit = 77;
    w.refuse_at = 3;
    assert(!mine_actor_sweep(&w.actor, &s, &hit) && hit == 77 && w.segments == 3);
    s.box = NULL;
    assert(!mine_actor_sweep(&w.actor, &s, &hit) && hit == 77 && w.segments == 3);
}

static void test_child(void)
{
    World w;
    MineActiveServices s = setup(&w);
    put(&w.actor, VARIANT, 2, 3);
    w.actor.flags = 9;
    put(w.models[0], 4, 4, HIDDEN);
    s.transform = NULL;
    assert(mine_actor_active(&w.actor, &s));
    assert(w.actor.flags == 0 && w.segments == 0 && w.transforms == 0);

    s = setup(&w);
    put(&w.actor, VARIANT, 2, 2);
    put(&w.actor, 0x74, 1, 93);
    s.segment = NULL;
    assert(mine_actor_active(&w.actor, &s));
    assert(w.actor.flags == 0x4400 && get(&w.actor, 0x102, 1) == 93 && w.transforms == 1);

    s = setup(&w);
    w.actor.flags = 1;
    put(&w.actor, VARIANT, 2, 2);
    w.activate = 1;
    w.hit_at = 2;
    assert(mine_actor_active(&w.actor, &s));
    assert(w.actor.flags == 0x4401 && w.event == 0x4007 && w.time == 32);
    assert(w.segments == 2 && get(&w.actor, 0x102, 1) == 15);

    s = setup(&w);
    put(&w.actor, VARIANT, 2, 3);
    s.event_time = NULL;
    w.hit_at = 1;
    assert(!mine_actor_active(&w.actor, &s));
    assert(w.event == 0x4007 && w.time == 0 && get(&w.actor, 0x102, 1) == 0);
    put(&w.actor, 0x64, 4, 0);
    assert(!mine_actor_active(&w.actor, &s) && w.transforms == 1);

    s = setup(&w);
    put(&w.actor, VARIANT, 2, 3);
    s.event_code = NULL;
    s.event_time = NULL;
    assert(mine_actor_active(&w.actor, &s));
    assert(w.segments == 5 && get(&w.actor, 0x102, 1) == 18);
    w.segments = 0;
    w.hit_at = 1;
    assert(!mine_actor_active(&w.actor, &s));
    assert(w.time == 0 && get(&w.actor, 0x102, 1) == 18);

    s = setup(&w);
    w.actor.probe_object_address = 0;
    put(&w.actor, VELOCITY, 4, 0x18000);
    assert(!mine_actor_active(&w.actor, &s));
    assert(get(&w.actor, POSITION, 4) == 0x10000 && w.transforms == 0);
}

int main(void)
{
    test_motion();
    test_parent_distance();
    test_sweep();
    test_child();
    return 0;
}

#include "floor_providers.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

Obj8014A2E4 D_801202A0[96];
SceneProbeVector D_801152A8;

typedef struct {
    Obj8014A2E4 *entry;
    SceneProbeVector position;
    SceneProbeVector normal;
    s32 hit;
} Reply;

static Reply replies[2];
static unsigned calls;
static SceneProbeVector start;
static SceneProbeVector *map_argument;
static s32 map_result;

static SceneProbeVector point(s16 seed)
{
    SceneProbeVector p = {seed, seed, (s16)(seed * 3), (s16)(seed - 19)};
    return p;
}

static int equal(SceneProbeVector a, SceneProbeVector b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.pad == b.pad;
}

s32 func_80135D20(u32 object, u32 geometry, SceneProbeVector *probe)
{
    assert(calls < sizeof(replies) / sizeof(replies[0]));
    const Reply *r = &replies[calls++];
    assert(object == r->entry->probe_object_address);
    assert(geometry == r->entry->probe_geometry_reference);
    assert(equal(*probe, start));
    *probe = r->position;
    D_801152A8 = r->normal;
    return r->hit;
}

s32 func_80134510(SceneProbeVector *probe)
{
    assert(probe == map_argument);
    probe->pad = start.pad;
    ++calls;
    return map_result;
}

static void prepare(Reply *r, unsigned slot, s16 seed)
{
    r->entry = &D_801202A0[slot];
    r->entry->active = 1;
    r->entry->flags = 0x400;
    r->entry->probe_object_address = UINT32_MAX - slot;
    r->entry->probe_geometry_reference = slot * 53;
    r->position = point(seed);
    r->normal = point((s16)-seed);
}

static void check_scans(int winner, unsigned expected_calls)
{
    for (int alternate = 0; alternate < 2; ++alternate) {
        SceneProbeVector best = start;
        SceneProbeVector normal = point(73);
        SceneProbeVector expected_position = start;
        SceneProbeVector expected_normal = normal;
        calls = 0;
        s32 found = alternate ? func_801502EC(NULL, &best, &normal)
                              : func_8014FFDC(NULL, &best, &normal);
        assert(calls == expected_calls);
        if (winner < 0) {
            expected_position.y = INT16_MAX;
        } else {
            expected_position = replies[winner].position;
            expected_normal = replies[winner].normal;
        }
        assert(found == (winner >= 0));
        assert(equal(best, expected_position));
        assert(equal(normal, expected_normal));
    }
}

int main(void)
{
    SceneProbeVector probe = point(-33);
    if (sizeof(uintptr_t) > sizeof(u32) && (uintptr_t)&probe <= UINT32_MAX) {
        return 77;
    }
    start = point(-57);
    map_argument = &probe;
    calls = 0;
    for (int hit = 0; hit <= 1; ++hit) {
        map_result = hit ? INT32_MIN : 0;
        probe.pad = 37;
        assert(func_80150150(NULL, &probe) == hit && probe.pad == start.pad);
        probe.pad = 37;
        assert(func_80150460(NULL, &probe) == hit && probe.pad == start.pad);
    }
    assert(calls == 4);

    Reply ignored;
    prepare(&ignored, 7, -65);
    ignored.entry->flags = 0;
    prepare(&replies[0], 23, -17);
    prepare(&replies[1], 81, -41);

    check_scans(-1, 2);
    replies[1].hit = 1;
    check_scans(1, 2);
    replies[0].hit = 1;
    check_scans(1, 2);
    replies[1].position.y = replies[0].position.y;
    check_scans(0, 2);

    replies[0].entry->active = 0;
    replies[1].entry->probe_geometry_reference = 0;
    check_scans(-1, 0);
    return 0;
}

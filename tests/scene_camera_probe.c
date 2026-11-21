#include "camera.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Exercise the caller and both existing clean smoothing wrappers. The synthetic
 * step and matrix providers check their inputs and produce chosen outputs. */
typedef struct {
    s16 from, target, rate, delta;
    int angular;
} ExpectedStep;
enum { SCALAR_STEPS = 8 };
static ExpectedStep steps[SCALAR_STEPS];
static int queued;
static SpatialPoint angles, transform_result;
static s32 depth, *active_transform;
static int cursor;
static unsigned random_state = 21;

static unsigned sample(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

enum { CASES = 4096 };
static s16 half(void) { return (s16)(sample() >> 16); }

static s16 add_step(s16 from, s16 target, s16 rate, int angular)
{
    assert(queued < SCALAR_STEPS);
    ExpectedStep *step = &steps[queued++];
    *step = (ExpectedStep){from, target, rate, half(), angular};
    int sum = from + step->delta;
    return angular ? (s16)(sum & 0xfff) : (s16)sum;
}

static s32 take_step(s16 from, s16 target, s16 rate, int angular)
{
    assert(cursor >= 0 && cursor < queued);
    ExpectedStep step = steps[cursor++];
    assert(step.from == from && step.target == target);
    assert(step.rate == rate && step.angular == angular);
    return step.delta;
}

s32 func_80012CB8(s16 from, s16 target, s16 rate)
{
    return take_step(from, target, rate, 0);
}

s32 func_80012B04(s16 from, s16 target, s16 rate)
{
    return take_step(from, target, rate, 1);
}

void func_8017D930(SceneVector *rotation, s32 *matrix)
{
    assert(cursor++ == SCALAR_STEPS && !memcmp(&rotation->position, &angles, sizeof angles));
    for (int i = 0; i < 8; ++i) matrix[i] = 0x1357090 + i;
    active_transform = matrix;
}

SceneVector *func_80048FBC(s32 *matrix, SceneVector *input, SceneVector *result)
{
    SceneVector *point = input;
    assert(cursor++ == SCALAR_STEPS + 1 && matrix == active_transform);
    for (int i = 0; i < 8; ++i) assert(active_transform[i] == 0x1357090 + i);
    assert(!point->position.x && !point->position.y && point->position.z == depth);
    result->position = transform_result;
    return result;
}

static int floor_divide_eight(int coordinate)
{
    return coordinate / 8 - (coordinate < 0 && coordinate % 8 != 0);
}

/* Cover every coordinate near the signed shift boundaries and both ends of
 * the halfword range. Two independent axes traverse the full product. */
enum { NEAR_ZERO = 35, COMPONENT_CASES = NEAR_ZERO + 3 };

static s16 component(unsigned index)
{
    unsigned slot = index % COMPONENT_CASES;
    if (slot < NEAR_ZERO) return (s16)((int)slot - NEAR_ZERO / 2);
    if (slot == NEAR_ZERO) return INT16_MIN;
    if (slot == NEAR_ZERO + 1) return INT16_MIN + 1;
    return INT16_MAX;
}

int main(void)
{
    _Static_assert(sizeof(SceneVector) == 8 && sizeof(WideVector) == 12, "vector layout");
    _Static_assert(offsetof(FacingActor, phase) == 5 && offsetof(FacingActor, distance) == 8 &&
                   offsetof(FacingActor, phase_value) == 20 && offsetof(FacingActor, rotation) == 24 &&
                   offsetof(FacingActor, target_rotation) == 32 && offsetof(FacingActor, offset) == 40 &&
                   offsetof(FacingActor, target_offset) == 46 && offsetof(FacingActor, camera) == 60 &&
                   offsetof(FacingActor, focus) == 72 && offsetof(FacingActor, facing) == 160, "actor layout");
    for (int j = 0; j < CASES; ++j) {
        FacingActor actor, expected;
        SceneVector position = {{half(), half(), half()}, half()};
        SceneVector position_before = position;
        s16 facing = half();
        for (unsigned i = 0; i < sizeof actor; ++i) ((u8 *)&actor)[i] = (u8)(sample() >> 24);
        memcpy(&expected, &actor, sizeof actor);
        queued = 0;
        expected.distance = add_step((s16)actor.distance, (s16)actor.target_distance, 4, 0);
        expected.depth = depth = add_step((s16)actor.depth, (s16)actor.phase_value, 4, 0);
        expected.rotation.position.x = add_step(actor.rotation.position.x, actor.target_rotation.position.x, 4, 1);
        expected.rotation.position.y = add_step(actor.rotation.position.y, actor.target_rotation.position.y, 4, 1);
        expected.rotation.position.z = add_step(actor.rotation.position.z, actor.target_rotation.position.z, 4, 1);
        expected.offset.x = add_step(actor.offset.x, actor.target_offset.x, 16, 0);
        expected.offset.y = add_step(actor.offset.y, actor.target_offset.y, 16, 0);
        expected.offset.z = add_step(actor.offset.z, actor.target_offset.z, 16, 0);
        assert(queued == SCALAR_STEPS);
        angles = expected.rotation.position;
        angles.y = (s16)(angles.y + facing);
        transform_result = (SpatialPoint){component((unsigned)j), component((unsigned)j / COMPONENT_CASES), half()};
        expected.focus = (WideVector){expected.offset.x + position.position.x, expected.offset.y + position.position.y,
                                      expected.offset.z + position.position.z};
        expected.camera = (WideVector){expected.focus.x + floor_divide_eight(transform_result.x),
            expected.focus.y + floor_divide_eight(transform_result.y), expected.focus.z + floor_divide_eight(transform_result.z)};
        cursor = 0;
        func_8017D2B8(&actor, facing, &position);
        assert(cursor == SCALAR_STEPS + 2 && !memcmp(&actor, &expected, sizeof actor));
        assert(!memcmp(&position, &position_before, sizeof position));
    }
    return 0;
}

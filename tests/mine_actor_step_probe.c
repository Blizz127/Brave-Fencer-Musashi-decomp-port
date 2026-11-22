#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mine_actor_step.h"

_Static_assert(sizeof(MineStepFixed) == 4, "actor position width");
_Static_assert(sizeof(MineStepActor) == 0x50, "actor motion extent");
_Static_assert(sizeof(SceneProbeVector) == 8, "offset extent");

static union {
    MineStepActor actor;
    struct { u16 type; SceneProbeVector offset; } fields;
} actor_with_offset;
static int overlapping;

static MineStepActor actor, entering, expected;
static SceneProbeVector offset, initial_offset, post_offset;
static u16 previous_point[3], current_point[3], corrected[3];
static u32 response;
static s32 requested_mode;
static int callback_count, rewrite_actor, rewrite_offset, rewrite_point;

static void point_matches(const MineActionPoint *p, const u16 xyz[3])
{
    assert(p->x == xyz[0] && p->y == xyz[1] && p->z == xyz[2]);
}

static u16 component(SceneProbeVector value, unsigned axis)
{
    if (axis == 0)
        return (u16)value.x;
    return axis == 1 ? (u16)value.y : (u16)value.z;
}

static void simulate_actor_change(MineStepActor *target)
{
    target->phase = 0x891;
    target->position[0].value = 0xdcba1021u;
    target->position[1].value = 0xabcd5678u;
    target->position[2].value = 0xef0189abu;
    target->velocity[0] = 0x66664444u;
    target->velocity[1] = 0xa1b2c3d4u;
    target->velocity[2] = 0x40201005u;
    target->previous[1].value = 0xdeb8ee51u;
    target->acceleration[2] = 0x13872456u;
}

s32 func_8012CEB0(MineActionPoint *from, MineActionPoint *to, s32 mode)
{
    assert(callback_count++ == 0 && mode == requested_mode);
    assert(from != to);
    if (overlapping) {
        assert(memcmp(&actor_with_offset.actor, &entering, sizeof(entering)) == 0);
        point_matches(from, previous_point);
        point_matches(to, current_point);
        if (rewrite_actor)
            simulate_actor_change(&actor_with_offset.actor);
        to->x = corrected[0];
        to->y = corrected[1];
        to->z = corrected[2];
        return (s32)response;
    }
    assert(memcmp(&actor, &entering, sizeof(actor)) == 0);
    assert(memcmp(&offset, &initial_offset, sizeof(offset)) == 0);
    point_matches(from, previous_point);
    point_matches(to, current_point);
    if (rewrite_actor)
        simulate_actor_change(&actor);
    if (rewrite_offset)
        offset = post_offset;
    from->x = 0x987;
    from->y = 0xfd43;
    from->z = 0x6b21;
    from->surface.flags = 0xc72f;
    if (rewrite_point) {
        to->x = corrected[0];
        to->y = corrected[1];
        to->z = corrected[2];
    }
    to->surface.flags = 0x7f32;
    return (s32)response;
}

static void check_step(s32 mode, u32 result, int change_actor, int change_offset, int change_point)
{
    static const u32 position[] = {0xfff0fff0u, 0x80004010u, 0x7777fabcu};
    static const u32 previous[] = {0xfff9a23bu, 0x8008bd2fu, 0x00380afdu};
    static const u32 velocity[] = {0xfffffff0u, 0x7fffffe0u, 0x88880090u};
    static const u32 acceleration[] = {0x00000031u, 0x00000060u, 0x8800fabcu};
    unsigned axis;

    memset(&actor, 0x5a, sizeof(actor));
    initial_offset = (SceneProbeVector){17, -26, 0x1234, 0x4321};
    post_offset = (SceneProbeVector){-87, 103, -193, -0x1234};
    offset = initial_offset;
    requested_mode = mode;
    response = result;
    rewrite_actor = change_actor;
    rewrite_offset = change_offset;
    rewrite_point = change_point;
    corrected[0] = 0x0003;
    corrected[1] = 0xffee;
    corrected[2] = 0xb903;

    for (axis = 0; axis < 3; ++axis) {
        actor.position[axis].value = position[axis];
        actor.previous[axis].value = previous[axis];
        actor.velocity[axis] = velocity[axis];
        actor.acceleration[axis] = acceleration[axis];
    }
    entering = actor;
    for (axis = 0; axis < 3; ++axis) {
        entering.velocity[axis] += acceleration[axis];
        entering.position[axis].value += entering.velocity[axis];
        previous_point[axis] = (u16)((previous[axis] >> 16) + component(initial_offset, axis));
        current_point[axis] = (u16)((entering.position[axis].value >> 16) + component(initial_offset, axis));
    }
    expected = entering;
    if (rewrite_actor)
        simulate_actor_change(&expected);
    for (axis = 0; axis < 3; ++axis) {
        u16 point = rewrite_point ? corrected[axis] : current_point[axis];
        u16 integer = (u16)(point - component(rewrite_offset ? post_offset : initial_offset, axis));
        expected.position[axis].value = ((u32)integer << 16) | (expected.position[axis].value & 0xffffu);
    }
    if (result & 0x6000u)
        expected.velocity[1] = 0;
    callback_count = 0;
    assert((u32)func_8012CC88(&actor, mode, &offset) == result);
    assert(callback_count == 1);
    assert(memcmp(&actor, &expected, sizeof(actor)) == 0);
    assert(memcmp(&offset, rewrite_offset ? &post_offset : &initial_offset, sizeof(offset)) == 0);
}

/* The caller can place the offset at actor + 2, where it spans the phase
   and both halves of X. Each point observes the offset at its own step. */
static void check_actor_offset(int change_actor)
{
    MineStepActor *view = &actor_with_offset.actor;
    u16 offsets[3];
    unsigned axis;

    assert((unsigned char *)&actor_with_offset.fields.offset ==
           (unsigned char *)view + 2);
    memset(view, 0x4f, sizeof(*view));
    view->phase = 0xfa31;
    view->position[0].value = 0x1234fffeu;
    view->position[1].value = 0x55550f31u;
    view->position[2].value = 0xf0000123u;
    view->velocity[0] = 2;
    view->velocity[1] = 0x99990001u;
    view->velocity[2] = 0x80ff0042u;
    view->acceleration[0] = 1;
    view->acceleration[1] = 0x88890003u;
    view->acceleration[2] = 0xff000013u;
    view->previous[0].value = 0xa5e34b21u;
    view->previous[1].value = 0xaa31f183u;
    view->previous[2].value = 0x9cdf19bau;

    offsets[0] = view->phase;
    offsets[1] = (u16)view->position[0].value;
    offsets[2] = (u16)(view->position[0].value >> 16);
    entering = *view;
    for (axis = 0; axis < 3; ++axis) {
        previous_point[axis] = (u16)((view->previous[axis].value >> 16) + offsets[axis]);
        entering.velocity[axis] += view->acceleration[axis];
        entering.position[axis].value += entering.velocity[axis];
    }
    offsets[1] = (u16)entering.position[0].value;
    offsets[2] = (u16)(entering.position[0].value >> 16);
    for (axis = 0; axis < 3; ++axis)
        current_point[axis] = (u16)((entering.position[axis].value >> 16) + offsets[axis]);

    expected = entering;
    if (change_actor)
        simulate_actor_change(&expected);
    offsets[0] = expected.phase;
    offsets[1] = (u16)expected.position[0].value;
    offsets[2] = (u16)(expected.position[0].value >> 16);
    corrected[0] = 0xc6ab;
    corrected[1] = 0x0621;
    corrected[2] = 0x435a;
    for (axis = 0; axis < 3; ++axis) {
        u16 integer = (u16)(corrected[axis] - offsets[axis]);
        expected.position[axis].value = ((u32)integer << 16) | (expected.position[axis].value & 0xffffu);
    }
    requested_mode = (s32)0xe1230001u;
    response = change_actor ? 0x4000 : 0x8000;
    if (change_actor)
        expected.velocity[1] = 0;
    rewrite_actor = change_actor;
    callback_count = 0;
    overlapping = 1;
    assert((u32)func_8012CC88(view, requested_mode, &actor_with_offset.fields.offset) == response);
    assert(callback_count == 1);
    assert(memcmp(view, &expected, sizeof(*view)) == 0);
    overlapping = 0;
}

int main(void)
{
    check_step(0, 0, 0, 0, 0);
    check_step(1, 0, 0, 0, 1);
    check_step(-3, 0x2000, 0, 0, 0);
    check_step(2, 0x4000, 0, 1, 1);
    check_step(-32768, 0x80000000u, 1, 1, 0);
    check_step(32767, 0x80008001u, 1, 0, 1);
    check_step(4, 0x60000001u, 1, 0, 0);
    check_step(7, 0x0000e000u, 1, 0, 1);
    check_step(-8, 0x80004002u, 1, 1, 1);
    check_step(-1, UINT32_MAX, 0, 1, 1);
    check_step(0x7fff0001, 0x4000, 1, 1, 0);
    check_step((s32)0x80000002u, 0x2000, 0, 1, 1);
    check_step((s32)0xffff8000u, 0, 0, 0, 1);
    check_actor_offset(0);
    check_actor_offset(1);
    return 0;
}

/* Compare signed face intersections to an independent wide-coordinate calculation. */
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "scene_box.h"

enum { Near = 41, Far = 75, Hit = 111, Step = 173, Alias = 225 };
enum { X, Y, Z };
static SceneProbeVector near_point, far_point, contact, movement;
static SceneProbeVector *alias_point;
static jmp_buf refusal;
static unsigned reason, normal_calls;
static u32 random_state = 0x91f246b3u;
u32 D_8017F68C = Near, D_8017F690 = Far, D_8017F694 = Hit, D_8017F698 = Step;
SceneProbeVector D_801152A8, D_80126720;
u32 D_801150D8;
typedef s32 (*Face)(SceneSurfaceBox *, s32, s16);
static Face axes[] = {func_801365B8, func_80136824, func_80136334};

SceneProbeVector *bfm_model_probe_point(u32 key)
{
    switch (key) {
    case Near: return &near_point;
    case Far: return &far_point;
    case Hit: return &contact;
    case Step: return &movement;
    case Alias: return alias_point;
    default: return 0;
    }
}
_Noreturn void bfm_model_box_invalid_point(u32 key)
{
    assert(key == 0 || key == Alias);
    reason = 1;
    longjmp(refusal, 1);
}
_Noreturn void bfm_model_box_invalid_bounds(void)
{
    reason = 3;
    longjmp(refusal, 1);
}
_Noreturn void bfm_model_box_divide_by_zero(void)
{
    reason = 2;
    longjmp(refusal, 1);
}
s32 func_80047E98(SceneProbeVector *input, SceneProbeVector *output)
{
    assert(input == &D_801152A8 && output == input);
    ++normal_calls;
    output->y = 73;
    return -17;
}
static uint32_t next_word(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}
static s16 component(SceneProbeVector point, unsigned axis)
{
    return axis == X ? point.x : axis == Y ? point.y : point.z;
}
static void put(SceneProbeVector *point, unsigned axis, s16 value)
{
    if (axis == X) point->x = value;
    else if (axis == Y) point->y = value;
    else point->z = value;
}
static void same(SceneProbeVector actual, SceneProbeVector expected)
{
    assert(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z);
    assert(actual.pad == expected.pad);
}
static SceneProbeVector generated(void)
{
    SceneProbeVector point;
    point.x = (s16)next_word(); point.y = (s16)next_word();
    point.z = (s16)next_word(); point.pad = (s16)next_word();
    return point;
}
static void setup(void)
{
    D_8017F68C = Near; D_8017F690 = Far; D_8017F694 = Hit; D_8017F698 = Step;
    near_point = generated(); far_point = generated(); movement = generated();
    contact = (SceneProbeVector){11, 23, 37, 57};
    D_801152A8 = (SceneProbeVector){17, -31, 3, -91};
    D_80126720 = (SceneProbeVector){51, -73, 67, 89};
    D_801150D8 = 0x12; normal_calls = reason = 0; alias_point = 0;
}
static void compare_face(SceneSurfaceBox *box, unsigned axis, s32 flags, s16 face)
{
    SceneProbeVector expected = contact, normal = D_801152A8, midpoint = D_80126720;
    const int32_t denominator = component(movement, axis);
    int32_t offset = (int32_t)face - component(near_point, axis);
    unsigned index;
    int hit = 1;
    assert(denominator != 0);
    for (index = 0; index < 3; ++index) {
        int64_t displacement = (int64_t)offset * component(movement, index) / denominator;
        s16 intersection = (s16)(component(near_point, index) + displacement);
        put(&expected, index, intersection);
        if (index != axis && (intersection < box->bounds[index * 2] ||
                              intersection > box->bounds[index * 2 + 1]))
            hit = 0;
    }
    if (hit) {
        for (index = 0; index < 3; ++index) {
            int32_t sum = (int32_t)box->bounds[index * 2] + box->bounds[index * 2 + 1];
            put(&midpoint, index, (s16)(sum < 0 ? -((-sum + 1) / 2) : sum / 2));
            put(&normal, index, index == axis ? ((flags & 1) ? 4095 : -4095) : 0);
            if (index == axis)
                put(&expected, index, (s16)(face + ((flags & 1) ? 2 : -2)));
            else if (flags & 0x8000)
                put(&expected, index, component(far_point, index));
        }
    }
    assert(axes[axis](box, flags, face) == hit);
    same(contact, expected); same(D_801152A8, normal); same(D_80126720, midpoint);
    assert(normal_calls == 0 && D_801150D8 == 0x12);
}
static void division_refusal(unsigned axis)
{
    SceneSurfaceBox box = {0, {-9, 12, -7, 21, -5, 15}};
    setup();
    put(&movement, axis, 0);
    if (setjmp(refusal) == 0) {
        (void)axes[axis](&box, 0, 7);
        assert(0);
    }
    assert(reason == 2);
    same(contact, (SceneProbeVector){axis == X ? 7 : 11, 23, 37, 57});
    same(D_801152A8, (SceneProbeVector){17, -31, 3, -91});
    same(D_80126720, (SceneProbeVector){51, -73, 67, 89});
}
static void check_backing(void)
{
    static _Alignas(SceneSurfaceBox) unsigned char storage[sizeof(SceneSurfaceBox) + 1];
    SceneSurfaceBox box = {0, {-10, 10, -12, 18, -8, 14}};
    setup();
    alias_point = (SceneProbeVector *)(storage + 1);
    D_8017F68C = Alias;
    if (setjmp(refusal) == 0) {
        (void)func_80135EB0(&box, 0);
        assert(0);
    }
    assert(reason == 1);
    if (setjmp(refusal) == 0) {
        (void)func_80135EB0((SceneSurfaceBox *)(storage + 1), 0);
        assert(0);
    }
    assert(reason == 3);
    if (setjmp(refusal) == 0) {
        (void)func_80136334(0, 0, 0);
        assert(0);
    }
    assert(reason == 3);
}
static void linked_box(void)
{
    SceneSurfaceBox box = {0, {-10, 10, -12, 18, -8, 14}};
    setup();
    near_point = (SceneProbeVector){-20, 2, -16, 21};
    far_point = (SceneProbeVector){0, 6, 0, 7};
    assert(func_80135EB0(&box, 0) == 1);
    same(contact, (SceneProbeVector){-10, 4, -10, 57});
    same(D_801152A8, (SceneProbeVector){0, 0, -4095, -91});
    same(D_80126720, (SceneProbeVector){0, 3, 3, 89});
    assert(normal_calls == 0 && D_801150D8 == 0x12);
    setup();
    near_point = (SceneProbeVector){8, 6, -4, 81};
    D_8017F690 = 0; D_8017F698 = 0;
    assert(func_80135EB0(&box, 0) == 1);
    same(contact, (SceneProbeVector){8, 6, -4, 57});
    same(D_801152A8, (SceneProbeVector){8, 73, -7, -91});
    same(D_80126720, (SceneProbeVector){0, 3, 3, 89});
    assert(normal_calls == 1 && D_801150D8 == 0x13);
    setup();
    near_point = (SceneProbeVector){80, 3, 5, 0};
    far_point = (SceneProbeVector){90, 6, 7, 0};
    D_8017F694 = D_8017F698 = 0;
    assert(func_80135EB0(&box, 0) == 0);
    same(contact, (SceneProbeVector){11, 23, 37, 57});
    D_8017F68C = 0;
    if (setjmp(refusal) == 0) {
        (void)func_80135EB0(&box, 0);
        assert(0);
    }
    assert(reason == 1);
    setup();
    near_point = (SceneProbeVector){0}; movement = (SceneProbeVector){0, 0, 10, 0};
    D_8017F694 = Alias; alias_point = &D_801152A8;
    assert(func_80136334(&box, 0, -8) == 1);
    same(D_801152A8, (SceneProbeVector){0, 0, -10, -91});
    setup();
    near_point = (SceneProbeVector){0}; movement = (SceneProbeVector){0, 0, 10, 0};
    D_8017F694 = Alias; alias_point = &D_801152A8;
    assert(func_80136334(&box, 1, 14) == 1);
    same(D_801152A8, (SceneProbeVector){0, 0, 4095, -91});
}
int main(void)
{
    unsigned trial, axis;
    _Static_assert(sizeof(SceneProbeVector) == 8 && sizeof(SceneSurfaceBox) == 16, "guest layouts");
    _Static_assert(offsetof(SceneSurfaceBox, bounds) == 4, "bounds follow guest address");
    for (trial = 0; trial < 3072; ++trial) {
        for (axis = 0; axis < 3; ++axis) {
            SceneSurfaceBox box = {0, {-32768, 32767, -32768, 32767, -32768, 32767}};
            s32 flags;
            s16 face;
            unsigned i;
            setup();
            flags = (s32)next_word(); face = (s16)next_word();
            if (trial & 1) {
                for (i = 0; i < 3; ++i) {
                    box.bounds[2*i] = (s16)-(int32_t)(next_word() % 16001u);
                    box.bounds[2*i+1] = (s16)(next_word() % 16001u);
                }
            }
            if (component(movement, axis) == 0) put(&movement, axis, 1);
            compare_face(&box, axis, flags, face);
        }
    }
    for (axis = 0; axis < 3; ++axis) division_refusal(axis);
    check_backing();
    linked_box();
    puts("9216 synthetic face intersections, split trap order, 32-bit slots, linked classifier and alias checks passed");
    return 0;
}

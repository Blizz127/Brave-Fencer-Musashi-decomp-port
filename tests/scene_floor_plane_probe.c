/* Synthetic geometry for the linked mine-member plane scan and dot product. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "floor_plane_probe.h"

enum { START = 0x381c, END = 0x6d72, HIT = 0x9324 };
u32 D_8017F68C = START;
u32 D_8017F690 = END;
u32 D_8017F694 = HIT;
SceneProbeVector D_801152A8;

static SceneProbeVector start, end, hit;
static unsigned refusals;
static const SceneProbeVector old_hit = { -107, 491, 87, 13 };
static const SceneProbeVector old_normal = { 91, -33, 12, 65 };
static FloorPlane planes[] = { { { 0, -1, 0 }, 71 }, { { 0, 0, 0 }, 39 } };
static s32 offsets[] = { 20, -12032, -12032 };
static s16 plain_record[] = { 17, 0, 0, 1, 1, 1, 1, 1, 1 };
static s16 flagged_record[] = { 17, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2 };

void *bfm_floor_address(u32 key)
{
    switch (key) {
    case START: return &start;
    case END: return &end;
    case HIT: return &hit;
    default: assert(0 && "unknown synthetic address"); return NULL;
    }
}

void bfm_floor_plane_divide_by_zero(void)
{
    refusals++;
}

static void reset(void)
{
    start = (SceneProbeVector) { 10, 10, -81, 947 };
    end = (SceneProbeVector) { 30, 30, 72, 19 };
    hit = old_hit;
    D_801152A8 = old_normal;
    planes[0] = (FloorPlane) { { 0, -1, 0 }, 71 };
    offsets[2] = -12032;
    refusals = 0;
}

static void expect_clipped_coordinates(void)
{
    assert(hit.x == 10 && hit.y == 20 && hit.z == -81);
    assert(hit.pad == old_hit.pad);
}

static void expect_accepted_normal(void)
{
    assert(D_801152A8.x == 0 && D_801152A8.y == -1);
    assert(D_801152A8.z == 0 && D_801152A8.pad == 71);
    assert(start.pad == -1 && refusals == 0);
}

static void test_plain_and_flagged_edges(void)
{
    reset();
    /* A plain record ends after the third side plane. */
    assert(func_801347A0(0, plain_record, planes, offsets) == 1);
    expect_clipped_coordinates();
    expect_accepted_normal();

    reset();
    assert(func_801347A0(-32768, flagged_record, planes, offsets) == 1);
    expect_clipped_coordinates();
    expect_accepted_normal();

    reset();
    /* One unit below the inclusive threshold rejects only the flagged entry. */
    offsets[2]--;
    assert(func_801347A0(-32768, flagged_record, planes, offsets) == 0);
    expect_clipped_coordinates();
    assert(memcmp(&D_801152A8, &old_normal, sizeof old_normal) == 0);
    assert(start.pad == 947 && refusals == 0);
}

static void test_zero_slope_refusal(void)
{
    reset();
    planes[0] = (FloorPlane) { { -1, 0, 0 }, 84 };
    /* The endpoints straddle this plane; the vertical divisor is zero. */
    assert(func_801347A0(0, plain_record, planes, offsets) == 0);
    assert(refusals == 1);
    assert(memcmp(&hit, &old_hit, sizeof old_hit) == 0);
    assert(memcmp(&D_801152A8, &old_normal, sizeof old_normal) == 0);
    assert(start.pad == 947);
}

static void test_signed_dot_wrap(void)
{
    const FloorPlane plane = { { 32767, 32767, 32767 }, 53 };
    const SceneProbeVector point = { 32767, 32767, 32767, 7 };
    int64_t unwrapped = INT64_C(3) * 32767 * 32767 - 19;
    assert(unwrapped > INT32_MAX);
    assert(func_80134A28(&plane, &point, -19) == unwrapped - (INT64_C(1) << 32));
}

int main(void)
{
#ifdef BFM_TEST_HIGH_ADDRESSES
    assert((uintptr_t)bfm_floor_address(START) > UINT32_MAX);
    assert((uintptr_t)bfm_floor_address(END) > UINT32_MAX);
    assert((uintptr_t)bfm_floor_address(HIT) > UINT32_MAX);
#endif
    test_plain_and_flagged_edges();
    test_zero_slope_refusal();
    test_signed_dot_wrap();
    return 0;
}

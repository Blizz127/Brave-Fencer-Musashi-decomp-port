#include "model_surface_classifier.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BOX_TAG (UINT32_C(1) << 31)
#define ROTATE (UINT32_C(1) << 30)
#define SCALE (UINT32_C(1) << 29)
#define IGNORE_RADIUS (UINT32_C(1) << 28)
enum { BOX_ID = 137, CLOSE_ID = 53, FAR_ID = 89 };

u32 D_8017F68C = CLOSE_ID;
u32 D_8017F690 = FAR_ID;
_Alignas(4) u8 D_8019F9CC[32];
SceneSurfaceBox D_8019F9EC;
static SceneSurfaceBox box;
static SceneSurfaceRecord model;
static SceneProbeVector near_point, far_point, close_out, far_out;
static const s16 scale[] = {-1024, 2048, -8192};
static char calls[12];
static unsigned used, point_reads, box_reads, zero_axis, corners;
static const void *expected_matrix;

static void called(char c)
{
    assert(used + 1 < sizeof(calls));
    calls[used++] = c;
    calls[used] = '\0';
}

static u16 half(const u8 *at)
{
    return (u16)((u16)at[0] | (u16)at[1] << 8);
}

static u32 word(const u8 *at)
{
    return (u32)half(at) | (u32)half(at + 2) << 16;
}

static void put(u8 *at, u32 value, unsigned width)
{
    for (unsigned i = 0; i < width; ++i)
        at[i] = (u8)(value >> (8 * i));
}

SceneProbeVector *bfm_model_probe_point(u32 address)
{
    ++point_reads;
    assert(address == CLOSE_ID || address == FAR_ID);
    return address == CLOSE_ID ? &close_out : &far_out;
}

SceneSurfaceBox *bfm_model_probe_box(u32 address)
{
    ++box_reads;
    assert(address == (BOX_TAG | BOX_ID));
    return &box;
}

void bfm_surface_zero_scale(unsigned axis)
{
    called('Z');
    zero_axis = axis;
}

void *func_80048EAC(void *matrix, void *result)
{
    called('C');
    assert(matrix == model.matrix && result == D_8019F9CC);
    return result;
}

SceneTransformVector *func_8004D504(void *matrix, SceneTransformVector *in, SceneTransformVector *out)
{
    called('T');
    assert(matrix == expected_matrix && in == out && in->x == near_point.x);
    out->x = -71;
    return out;
}

SceneTransformVector *func_80048B6C(SceneTransformVector *in, SceneTransformVector *out)
{
    called('R');
    assert(in == out && in->x == far_point.x);
    out->x = 113;
    return out;
}

void func_8004914C(void *matrix)
{
    called('S');
    const u8 *bytes = matrix;
    assert(matrix != expected_matrix && matrix != model.matrix && matrix != D_8019F9CC);
    assert(word(bytes) == (u32)(s32)scale[0] && word(bytes + 8) == (u32)(s32)scale[1]);
    assert(half(bytes + 16) == (u16)scale[2]);
}

SceneTransformVector *func_8004864C(SceneProbeVector *in, SceneTransformVector *out)
{
    called('B');
    assert(corners < 2 && in->x == box.bounds[corners] && in->y == box.bounds[corners + 2]);
    assert(in->z == box.bounds[corners + 4]);
    ++corners;
    out->x = out->y = out->z = (s32)corners * 17;
    return out;
}

static void check(u32 flag, int code, const char *trace)
{
    used = point_reads = box_reads = corners = 0;
    calls[0] = '\0';
    close_out.x = far_out.x = -311;
    expected_matrix = code == 4 ? D_8019F9CC : model.matrix;
    assert(func_80135480(&model, flag | BOX_ID, &near_point, &far_point) == code);
    assert(strcmp(calls, trace) == 0);
    assert(point_reads == (code ? 2u : 0u));
    assert(box_reads == (code >= 3));
    if (code == 0) {
        assert(close_out.x == -311 && far_out.x == -311);
    } else {
        int rotated = code == 2 || code == 4;
        assert(close_out.x == (rotated ? -71 : near_point.x));
        assert(far_out.x == (rotated ? 113 : far_point.x));
    }
    if (code >= 3) {
        assert(corners == 2 && D_8019F9EC.next == 0);
        for (unsigned i = 0; i < 6; ++i)
            assert(D_8019F9EC.bounds[i] == (i % 2 ? 34 : 17));
    }
}

int main(void)
{
    if (sizeof(uintptr_t) > sizeof(u32) &&
        ((uintptr_t)&model <= UINT32_MAX || (uintptr_t)&close_out <= UINT32_MAX))
        return 77;
    for (unsigned i = 0; i < 3; ++i)
        put(model.prefix + 24 + 2 * i, (u16)scale[i], 2);
    for (unsigned i = 0; i < 6; ++i)
        box.bounds[i] = (s16)(i * 11 + 7);
    near_point.x = 512;
    far_point.x = -43;
    check(0, 1, "");
    ++near_point.x;
    check(0, 0, "");
    check(IGNORE_RADIUS, 1, "");
    near_point.x = -21;
    check(SCALE, 1, "");
    check(SCALE | ROTATE, 2, "TR");
    check(BOX_TAG | SCALE, 3, "SBB");
    check(BOX_TAG | SCALE | ROTATE, 4, "CTRSBB");
    assert(word(D_8019F9CC) == (u32)(16777216 / scale[0]));
    assert(word(D_8019F9CC + 8) == (u32)(16777216 / scale[1]));
    assert(half(D_8019F9CC + 16) == (u16)(16777216 / scale[2]));
    for (unsigned i = 0; i < 3; ++i) {
        put(model.prefix + 24 + 2 * i, 0, 2);
        memset(D_8019F9CC, 0x65, sizeof(D_8019F9CC));
        zero_axis = 99;
        check(BOX_TAG | SCALE | ROTATE, 0, "Z");
        assert(zero_axis == i && D_8019F9CC[4] == 0x65);
        assert(word(D_8019F9CC) == (i ? (u32)(16777216 / scale[0]) : UINT32_C(0x65656565)));
        put(model.prefix + 24 + 2 * i, (u16)scale[i], 2);
    }
    return 0;
}

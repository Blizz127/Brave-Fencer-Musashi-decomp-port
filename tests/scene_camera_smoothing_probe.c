#include "camera.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

extern s32 func_80012B04(s16, s16, s16);
extern s32 func_80012CB8(s16, s16, s16);
static unsigned seed = 3050;

static s16 sample(void)
{
    seed = 1664525u * seed + 1013904223u;
    return (s16)(seed >> 16);
}

static int step(s16 from, s16 to, int rate, int angle)
{
    int delta = (s16)(to - from);
    if (angle && delta > 2048) delta -= 4096;
    if (angle && delta < -2048) delta += 4096;
    for (unsigned tries = 4; tries && rate; --tries, rate /= 2) {
        int quotient = (s16)(delta / rate);
        if (quotient) return quotient;
    }
    return delta;
}

static SpatialPoint expected_rotation, matrix_reply;
static s16 expected_depth;
static int callbacks;
static s32 *matrix_address;

void func_8017D930(SceneVector *rotation, s32 *matrix)
{
    assert(callbacks++ == 0);
    assert(!memcmp(&rotation->position, &expected_rotation, sizeof expected_rotation));
    matrix[0] = 0x1234;
    matrix[7] = -987;
    matrix_address = matrix;
}

SceneVector *func_80048FBC(s32 *matrix, SceneVector *input, SceneVector *result)
{
    assert(callbacks++ == 1);
    assert(matrix == matrix_address && matrix[0] == 0x1234 && matrix[7] == -987);
    assert(input->position.x == 0 && input->position.y == 0 && input->position.z == expected_depth);
    result->position = matrix_reply;
    return result;
}

static s16 angle_next(s16 from, s16 to)
{
    return (s16)((from + step(from, to, 4, 1)) & 4095);
}

static s16 next(s16 from, s16 to, int rate)
{
    return (s16)(from + step(from, to, rate, 0));
}

static int downscale(s16 value)
{
    return value / 8 - (value < 0 && value % 8 != 0);
}

int main(void)
{
    static const s16 edges[] = {-32768, -32767, -8193, -4097, -4096, -2049, -2048, -2047,
        -17, -16, -15, -9, -8, -7, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 7, 8, 9,
        15, 16, 17, 2047, 2048, 2049, 4095, 4096, 4097, 32766, 32767};
    const unsigned count = sizeof edges / sizeof edges[0];
    for (unsigned i = 0; i < count; ++i)
        for (unsigned j = 0; j < count; ++j)
            for (unsigned k = 0; k < count; ++k) {
                assert(func_80012CB8(edges[i], edges[j], edges[k]) == step(edges[i], edges[j], edges[k], 0));
                assert(func_80012B04(edges[i], edges[j], edges[k]) == step(edges[i], edges[j], edges[k], 1));
            }
    for (int i = 0; i < 4096; ++i) {
        FacingActor actor, expected;
        for (unsigned j = 0; j < sizeof actor; ++j) ((u8 *)&actor)[j] = (u8)sample();
        memcpy(&expected, &actor, sizeof actor);
        SceneVector position = {{sample(), sample(), sample()}, sample()};
        SceneVector original = position;
        s16 facing = sample();
        expected.distance = next((s16)actor.distance, (s16)actor.target_distance, 4);
        expected.depth = expected_depth = next((s16)actor.depth, (s16)actor.phase_value, 4);
        expected.rotation.position.x = angle_next(actor.rotation.position.x, actor.target_rotation.position.x);
        expected.rotation.position.y = angle_next(actor.rotation.position.y, actor.target_rotation.position.y);
        expected.rotation.position.z = angle_next(actor.rotation.position.z, actor.target_rotation.position.z);
        expected_rotation = expected.rotation.position;
        expected_rotation.y = (s16)(expected_rotation.y + facing);
        expected.offset.x = next(actor.offset.x, actor.target_offset.x, 16);
        expected.offset.y = next(actor.offset.y, actor.target_offset.y, 16);
        expected.offset.z = next(actor.offset.z, actor.target_offset.z, 16);
        expected.focus = (WideVector){position.position.x + expected.offset.x, position.position.y + expected.offset.y,
                                      position.position.z + expected.offset.z};
        matrix_reply = (SpatialPoint){sample(), sample(), sample()};
        expected.camera = (WideVector){expected.focus.x + downscale(matrix_reply.x),
            expected.focus.y + downscale(matrix_reply.y), expected.focus.z + downscale(matrix_reply.z)};
        callbacks = 0;
        func_8017D2B8(&actor, facing, &position);
        assert(callbacks == 2 && !memcmp(&expected, &actor, sizeof actor));
        assert(!memcmp(&original, &position, sizeof position));
    }
    return 0;
}

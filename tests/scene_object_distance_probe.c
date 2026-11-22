#include <assert.h>
#include "object_distance.h"
#include "scene_object_probes.h"

static s16 first[8], second[8];
static u32 components[3];
static s32 result;
static unsigned clock_step;

void func_80049324(DistanceVector *delta, DistanceVector *square)
{
    assert(clock_step++ == 0);
    assert(delta->x == first[3] - second[3]);
    assert(delta->y == first[5] - second[5]);
    assert(delta->z == first[7] - second[7]);
    square->x = (s32)components[0];
    square->y = (s32)components[1];
    square->z = (s32)components[2];
}
s32 func_80047D3C(s32 arg)
{
    assert(clock_step++ == 1);
    assert((u32)arg == components[0] + components[1] + components[2]);
    return result;
}
int main(void)
{
    u32 seed = 257;
    for (unsigned i = 0; i < 4096; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            seed = seed * 1664525u + 1013904223u;
            first[j] = (s16)seed;
            second[j] = (s16)(seed >> 16);
        }
        for (unsigned j = 0; j < 3; ++j) {
            seed = seed * 1664525u + 1013904223u;
            components[j] = seed;
        }
        s32 limit = (s32)(seed ^ i);
        result = (s32)(seed ^ (seed >> 6));
        if (i % 5 == 0)
            result = limit & 0xffff;
        else if (i % 5 == 1)
            result = (limit & 0xffff) - 1;
        clock_step = 0;
        assert(func_8014C278((char *)first, (char *)second, limit) == (result < (limit & 0xffff)));
        assert(clock_step == 2);
        clock_step = 0;
        assert(func_80013328((char *)first + 4, (char *)second + 4) == result);
        assert(clock_step == 2);
    }
}

/* Sum the three squared coordinate differences from two position records. */
#include "object_distance.h"

extern void func_80049324(DistanceVector *input, DistanceVector *output);

s32 func_80013350(void *a, void *b)
{
    s16 *first = a;
    s16 *second = b;
    DistanceVector delta, square;

    delta.x = first[1] - second[1];
    delta.y = first[3] - second[3];
    delta.z = first[5] - second[5];
    func_80049324(&delta, &square);
    return (s32)((u32)square.x + (u32)square.y + (u32)square.z);
}

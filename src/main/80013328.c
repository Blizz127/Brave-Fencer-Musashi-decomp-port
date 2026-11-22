/* Distance between two fixed-point position records. */
#include "object_distance.h"

extern s32 func_80047D3C(s32 squared_distance);

s32 func_80013328(char *a, char *b)
{
    return func_80047D3C(func_80013350(a, b));
}

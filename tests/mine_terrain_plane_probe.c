#include "psx_types.h"

#include <assert.h>
#include <limits.h>

s32 func_80134310(s16 *, s16 *, s32);
s32 func_8013435C(s16 *, s16 *, s32, s16 *);

int main(void)
{
    s16 point[3] = {-10, 3, 12}, normal[3] = {2, -9, 4};
    s16 face[3] = {0, -0xe00, 0}, zero[3] = {0, 0, 0};
    s16 opposite[3] = {-32768, -32768, 0};

    assert(func_80134310(point, normal, -2) == -1);
    assert(func_80134310(opposite, opposite, 0) == INT_MIN);
    assert(func_80134310(opposite, opposite, INT_MIN) == 0);
    assert(func_8013435C(zero, normal, -0x2f00, face) == 0);
    assert(func_8013435C(zero, normal, -0x2f01, face) == 1);
    face[1] = -0xe01;
    assert(func_8013435C(zero, normal, -0x1800, face) == 0);
    assert(func_8013435C(zero, normal, -0x1801, face) == 1);
    assert(func_8013435C(opposite, opposite, 0, opposite) == 1);
    return 0;
}

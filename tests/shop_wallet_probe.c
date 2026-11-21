#include <assert.h>
#include <stdint.h>

#include "psx_types.h"

s32 D_80078E8C;
s32 func_8018C914(s32 delta);

static void check(s32 before, s32 delta)
{
    int64_t sum = (int64_t)before + delta;
    s32 status;

    if (sum > INT32_MAX)
        sum -= INT64_C(4294967296);
    else if (sum < INT32_MIN)
        sum += INT64_C(4294967296);

    D_80078E8C = before;
    status = func_8018C914(delta);
    if (sum < 0) {
        assert(status == 1 && D_80078E8C == before);
    } else {
        s32 expected = sum > 99999 ? 99999 : (s32)sum;
        assert(status == 0 && D_80078E8C == expected);
    }
}

int main(void)
{
    s32 delta;
    check(100, -8);
    check(7, -8);
    check(8, -8);
    check(0, INT32_MIN);
    check(-1, INT32_MIN);
    check(99999, INT32_MAX);

    for (delta = -32768; delta <= 32767; ++delta) {
        check(100, delta);
        check(99999, delta);
        check(INT32_MAX, delta);
        check(INT32_MIN, delta);
    }
    return 0;
}

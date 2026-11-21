/* func_80012CB8: step from `from` toward `to`, with an attempt budget of 4. */
#include "psx_types.h"

s16 func_80012D0C(s16 from, s16 to, s16 divisor, s16 *attempts);

s32 func_80012CB8(s16 from, s16 to, s16 divisor) {
    s16 attempts = 4;

    if (from == to) {
        return 0;
    }
    return func_80012D0C(from, to, divisor, &attempts);
}

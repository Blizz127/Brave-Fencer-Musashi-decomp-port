/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 0 when from == to, otherwise func_80012B58(from, to, divisor, &state) with state preset to 4. */
#include "psx_types.h"

extern s16 func_80012B58(s16 from, s16 to, s16 divisor, s16 *state);

s32 func_80012B04(s16 from, s16 to, s16 divisor)
{
    s16 state = 4;

    if (from == to) {
        return 0;
    }
    return func_80012B58(from, to, divisor, &state);
}

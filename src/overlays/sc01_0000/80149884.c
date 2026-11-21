/* Fresh own-retail signed-halfword gate and unsigned query. */
#include "psx_types.h"
extern s16 D_800B9A08;
extern u32 func_8016F1AC(void);

s32 func_80149884(void)
{
    s32 result;
    if (D_800B9A08 == 0x308F) {
        result = 0;
    } else {
        result = (func_8016F1AC() == 0U);
    }
    return result;
}

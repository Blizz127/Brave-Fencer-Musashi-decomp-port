/* Own retail leaf; retain the qualified root API.
 * Input must provide readable bytes through offset0x1C4.
 * Incoming native backing and parent compatibility are unproved.
 * Source-only and unselected; no runtime execution or admission.
 */
#include "psx_types.h"
u8 func_8014B5B8(u8 *p)
{
    return p[0x1C4];
}

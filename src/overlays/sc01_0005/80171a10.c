/* Own retail leaf; retain the qualified root API.
 * Input must provide writable bytes through offset0x217.
 * Incoming native backing and parent compatibility are unproved.
 * Source-only and unselected; no runtime execution or admission.
 */
#include "psx_types.h"
void func_80171A10(u8 *p)
{
    p[0x217] = 2;
}

/* Own retail leaf; retain the qualified root API.
 * Input must provide writable bytes through offset0x220.
 * Incoming native backing and parent compatibility are unproved.
 * Source-only and unselected; no runtime execution or admission.
 */
#include "psx_types.h"
void func_801470A0(u8 *p)
{
    p[0x220] = 1;
}

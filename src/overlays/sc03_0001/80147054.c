/* Own retail: set the byte at +0x4C to one, under the existing void API.
 * Input must provide writable byte storage through offset0x4C.
 * Native backing/caller extent are unproved; source-only and unselected.
 */
#include "psx_types.h"
void func_80147054(u8 *p)
{
    p[0x4C] = 1;
}

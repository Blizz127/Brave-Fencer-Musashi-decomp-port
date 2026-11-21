/* Own retail: store byte one at +0x217 under the existing void API.
 * Input must provide writable byte storage through offset0x217.
 * Native backing/caller extent are unproved; source-only and unselected.
 */
#include "psx_types.h"
void func_80171A04(u8 *p)
{
    p[0x217] = 1;
}

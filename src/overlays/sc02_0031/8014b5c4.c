/* Own retail: store the low byte of the OR of two full-word arguments.
 * Input must provide writable byte storage through offset0x1C7.
 * Native backing/caller extent are unproved; source-only and unselected.
 */
#include "psx_types.h"
void func_8014B5C4(u8 *p, u32 a, u32 b)
{
    a |= b;
    p[0x1C7] = (u8)a;
}

/* Own retail: full-word unsigned offset, then a 12-bit angle mask.
 * Source-only and unselected; no runtime or input-range assumption.
 */
#include "psx_types.h"
typedef char BfmAngleWordWidth[
    (sizeof(u32) == 4 && sizeof(s32) == 4) ? 1 : -1];
s32 func_80148FE0(s32 a)
{
    return (s32)(((u32)a - 0x600U) & 0xFFFU);
}

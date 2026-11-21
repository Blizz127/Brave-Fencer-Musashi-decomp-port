/* Own retail: wrapped full-word +0x600 followed by a 12-bit angle mask.
 * Source-only and unselected; no runtime or caller-domain claim.
 */
#include "psx_types.h"
typedef char BfmAngleWordWidth[
    (sizeof(u32) == 4 && sizeof(s32) == 4) ? 1 : -1];
s32 func_80148FA0(s32 a)
{
    return (s32)(((u32)a + 0x600U) & 0xFFFU);
}

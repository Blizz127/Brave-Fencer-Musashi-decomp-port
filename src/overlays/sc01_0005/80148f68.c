/* Own retail: sign-extend the low halfword of the full incoming s32.
 * The qualified s16 return uses the checked compiler narrowing convention.
 * Source-only and unselected; no runtime or caller backing claim.
 */
#include "psx_types.h"
typedef char BfmSignedHalfwordNarrowing[
    (sizeof(s16) == 2 && sizeof(s32) == 4 &&
     (s16)0x8000 == -32768 && (s16)0xFFFF == -1) ? 1 : -1];
s16 func_80148F68(s32 a)
{
    return (s16)a;
}

/* Fresh own-retail derivation; requires writable byte storage through +0x222. */
#include "psx_types.h"

void func_801470B4(u8 *o)
{
    o[0x222] = 1;
}

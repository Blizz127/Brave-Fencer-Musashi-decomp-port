/* Owner re-derivation from retail assembly (clean input set). */
/* If D_8011DB28 (function-pointer global) is set, call it with o+0xA0; always set the u8
 * state flag at o+0xDE to 0x5A. */
#include "psx_types.h"

extern void (*D_8011DB28)(u8 *);

void func_8015369C(u8 *o)
{
    if (D_8011DB28 != 0) {
        D_8011DB28(o + 0xA0);
    }
    o[0xDE] = 0x5A;
}

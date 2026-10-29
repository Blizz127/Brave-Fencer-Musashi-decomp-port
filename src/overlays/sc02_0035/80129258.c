/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u16 type;
    /* 0x02 */ u8 pad02[0x36];
} Slot80129258; /* size 0x38 */

extern u8 D_8011DB30[];

void func_80129258(void) {
    Slot80129258 *p = ((Slot80129258 *)&D_8011DB30);
    s32 i;

    for (i = 127; i >= 0; i--) {
        p->type = 0;
        p++;
    }
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u16 f;
    u8 pad[0x36];
} E801291C0;
extern u8 D_8011DB30[];
extern void func_80016714(void *, s32);

E801291C0 *func_801291C0(void) {
    E801291C0 *e = ((E801291C0 *)&D_8011DB30);
    s32 i;
    for (i = 0; i < 0x80; i++, e++) {
        if (e->f == 0) {
            func_80016714(e, 0x38);
            return e;
        }
    }
    return 0;
}

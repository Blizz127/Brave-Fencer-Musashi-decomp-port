/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 f;
    u8 pad[3];
    s32 g;
    u8 rest[0x24];
} E801468C8;
extern E801468C8 D_8011F750[];

E801468C8 *func_801468C8(s32 a, u8 b) {
    s32 i;
    for (i = 0; i < 2; i++) {
        if (D_8011F750[i].f == 0) {
            D_8011F750[i].g = a;
            D_8011F750[i].f = b;
            return &D_8011F750[i];
        }
    }
    return 0;
}

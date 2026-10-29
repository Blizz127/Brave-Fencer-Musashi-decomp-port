/* Owner re-derivation from retail assembly (clean input set). */
/* Marks the current entry as finished once, stops the pending sound (func_800415A8) if any, and sets the done flag. */
#include "psx_types.h"

typedef struct {
    s16 normal;
    s16 alt;
} Entry800C5328;

extern void func_800415A8(s16);
extern s16 D_800A4688;
extern u8 D_800A4698[];
extern s16 D_800A46A2;
extern u8 D_800A46B0;
extern Entry800C5328 D_800C5328[];

void func_8002F67C(void) {
    if (D_800A4698[0] == 0) {
        D_800C5328[D_800A4688].normal = -1;
        D_800A4698[0] = 1;
    }
    if (D_800A46A2 >= 0) {
        func_800415A8(D_800A46A2);
        D_800A46A2 = -1;
    }
    if (D_800A46B0 == 0) {
        D_800A46B0 = 1;
    }
}

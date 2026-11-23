/* Owner re-derivation from retail assembly (clean input set). */
/* Latches a controller's button state for the frame (previous, newly pressed, held history) according to its pad type, then runs the type-specific handler. */
#include "controller_report.h"

extern void func_80016714(s8 *, s32);
extern void func_80018F20(u8 *);

void func_80018CE8(PadRecord *pad) {
    s32 i;

    if (pad == &D_80078D98[1]) {
        func_80016714((s8 *)pad, 0x4C);
        return;
    }
    switch (pad->type) {
    case 0x41:
        for (i = 0; i < 2; i++) {
            pad->older[i] = pad->previous[i];
            pad->pressed[i] = pad->current[i] & (pad->current[i] ^ pad->previous[i]);
            pad->previous[i] = pad->current[i];
        }
        return;
    case 0x53:
        for (i = 0; i < 2; i++) {
            pad->older[i] = pad->previous[i];
            pad->pressed[i] = pad->current[i] & (pad->current[i] ^ pad->previous[i]);
            pad->previous[i] = pad->current[i];
        }
        for (; i < 8; i++) {
            pad->older[i] = pad->previous[i];
            pad->previous[i] = pad->current[i];
        }
        func_80018F20((u8 *)pad);
        return;
    case 0x73:
        for (i = 0; i < 2; i++) {
            pad->older[i] = pad->previous[i];
            pad->pressed[i] = pad->current[i] & (pad->current[i] ^ pad->previous[i]);
            pad->previous[i] = pad->current[i];
        }
        for (; i < 8; i++) {
            pad->older[i] = pad->previous[i];
            pad->previous[i] = pad->current[i];
        }
        func_80018F20((u8 *)pad);
        return;
    }
    func_80016714((s8 *)pad, 0x4C);
}

/* Owner re-derivation from retail assembly (clean input set). */
/* Latches a controller's button state for the frame (previous, newly pressed, held history) according to its pad type, then runs the type-specific handler. */
#include "psx_types.h"

typedef struct {
    u8 type;
    u8 pad1[0x2A - 1];
    u8 current[8];
    u8 previous[8];
    u8 pressed[8];
    u8 older[8];
} PadState;

extern void func_80016714(PadState *, s32);
extern void func_80018F20(PadState *);
extern PadState D_80078DE4;

void func_80018CE8(PadState *pad) {
    s32 i;

    if (pad == &D_80078DE4) {
        func_80016714(pad, 0x4C);
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
        func_80018F20(pad);
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
        func_80018F20(pad);
        return;
    }
    func_80016714(pad, 0x4C);
}

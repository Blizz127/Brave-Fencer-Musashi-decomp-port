/* Start a game track or continue its pending bank request. The sound
 * services own the 32-bit bank address and return a signed 16-bit handle. */
#include "game_track_state.h"

extern u8 D_80064D49[][12], D_80064D4A[][12], D_80064D4E[][12];
extern s16 D_800A4EF6, D_800C5328[][2], D_800C532A[][2];
extern void func_80031A98(void);
extern s32 func_800419B0(s32 handle);
extern s32 func_80030730(void);
extern s32 func_80030A14(void);
extern void func_800415A8(s32 handle);

s32 func_8002FF0C(s32 track, s32 header)
{
    s32 slot;
    s32 finished;

    if (D_800A4638.phase == 0) {
        slot = D_80064D49[track][0];
        if (D_800A4638.slots[slot].other_type && D_800A4638.slots[slot].other_type != D_80064D4A[track][0]) {
            D_800C532A[D_800A4638.slots[slot].other_track][0] = -1;
            D_800A4638.slots[slot].other_type = 0;
            D_800A4638.slots[slot].other_track = 0;
        }
        if (!D_800A4638.slots[slot].empty) {
            D_800C5328[D_800A4638.slots[slot].track][0] = -1;
            func_80031A98();
        }
        D_800A4638.slot = slot;
        D_800A4638.slots[slot].track = track;
        D_800A4638.track = track;
        D_800A4638.header = header;
        D_800A4638.slots[slot].header = header;
        if (D_80064D4E[track][0])
            D_800A4EF6 = D_80064D4E[track][0];
        if ((D_800A4638.slots[slot].handle = D_800A4638.handle =
                 func_80041A80(header, -1, D_800A4638.slots[slot].address)) == -1 ||
            (s16)func_800419B0(D_800A4638.handle) == -1) {
            D_800A4638.slots[slot].empty = 1;
            return 1;
        }
        D_800A4638.phase = 1;
    }
    finished = 0;
    switch (D_800A4638.phase) {
    case 1:
        finished = func_80030730();
        break;
    case 2:
        finished = func_80030A14();
        break;
    }
    if (finished) {
        func_800415A8(D_800A4638.handle);
        D_800A4638.handle = -1;
        D_800A4638.slots[D_800A4638.slot].handle = -1;
        return 1;
    }
    return 0;
}

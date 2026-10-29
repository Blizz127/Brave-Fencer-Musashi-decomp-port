/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame update of the two-option title/continue menu: blinking prompt, cursor movement with sound, selection, and an idle timeout that picks option 4. */
#include "psx_types.h"

extern void func_800118AC(void);
extern s32 func_800149E0(s32);
extern void func_80014B10(s32);
extern void func_8002D4C8(s32, s32);
extern void func_800CF3E8(void);
extern void func_800CFE00(void);
extern s16 D_800EC678;
extern s32 D_800EC67C;
extern s32 D_800EC680;
extern s32 D_800EC690;
extern s32 D_800EC694;
extern s32 D_800EC698;

void func_800CF104(void) {
    s32 pad;

    pad = func_800149E0(0);
    func_80014B10(0);
    switch (D_800EC690) {
    case 0:
        break;
    case 1:
        D_800EC694 = (D_800EC698 & 0x1F) < 24;
        D_800EC698++;
        if (pad & 0x840) {
            func_8002D4C8(0xC6E, 0);
            D_800EC690++;
        } else {
            D_800EC67C++;
        }
        break;
    case 3:
        func_800CFE00();
        D_800EC690 = 2;
        break;
    case 2:
        if (pad & 0x4000) {
            func_8002D4C8(0x9C1, 0);
            if (++D_800EC678 >= 2) {
                D_800EC678 = 0;
            }
            D_800EC67C = 0;
        } else if (pad & 0x1000) {
            func_8002D4C8(0x9C1, 0);
            if (--D_800EC678 < 0) {
                D_800EC678 = 1;
            }
            D_800EC67C = 0;
        } else if (pad & 0x840) {
            func_8002D4C8(0x9C0, 0);
            D_800EC680 = D_800EC678;
            func_800118AC();
        }
        break;
    }
    func_800CF3E8();
    if (D_800EC67C >= 451) {
        D_800EC680 = 4;
        func_800118AC();
    }
}

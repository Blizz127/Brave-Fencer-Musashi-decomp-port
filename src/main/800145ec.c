/* func_800145EC: select display-mode table entry `i`, then set up the display. */
#include "psx_types.h"

typedef struct {
    u16 w;
    u16 h;
    u16 flags;
} Mode;

extern Mode D_80062A3C[];
extern Mode D_800AF7BC;
extern void func_800525DC(s32, s32, s32, s32, s32);
extern void func_80053178(s32, s32, s32, s32);
extern void func_80059234(s32);
extern void func_80053218(void);
extern void func_80014774(void);
extern void func_800147B8(void);

void func_800145EC(u16 i) {
    D_800AF7BC.w = D_80062A3C[i].w;
    D_800AF7BC.h = D_80062A3C[i].h;
    D_800AF7BC.flags = D_80062A3C[i].flags;
    func_800525DC(D_800AF7BC.w, D_800AF7BC.h, D_800AF7BC.flags | 4, 0, 0);
    if (D_800AF7BC.h == 0x1E0) {
        func_80053178(0, 0, 0, 0);
    } else {
        func_80053178(0, D_800AF7BC.h, 0, 0);
    }
    func_80059234(1);
    func_80053218();
    func_80014774();
    func_800147B8();
}

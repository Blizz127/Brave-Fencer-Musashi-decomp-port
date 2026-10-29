/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_800CF3E8, then lowers the counter D_800EC68C by 4; once it goes negative clears the 640x480 area (a four
 * half-word rectangle on the stack) through func_80059888 and runs func_800118AC. */
#include "psx_types.h"

typedef struct {
    s16 x, y, w, h;
} Rect;

extern s32 D_800EC68C;
extern void func_800CF3E8(void);
extern void func_80059888(Rect *r, s32 a, s32 b, s32 c);
extern void func_800118AC(void);

void func_800CF300(void)
{
    Rect r;

    func_800CF3E8();
    D_800EC68C -= 4;
    if (D_800EC68C < 0) {
        r.x = 0;
        r.y = 0;
        r.w = 0x280;
        r.h = 0x1E0;
        func_80059888(&r, 0, 0, 0);
        func_800118AC();
    }
}

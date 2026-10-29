/* Owner re-derivation from retail assembly (clean input set). */
/* Display setup for the 640x480 movie mode: sets the display size half-words D_800AF7BC/D_800AF7BE/D_800AF7C0, runs
 * func_800527C4(640, 480, 5, 0, 0), func_80053EEC(0, 0, 0, 0), func_80059234(1), func_80053218 and func_800147B8, clears the
 * top band (0,0,640,20) and the bottom band (0,460,640,20) to black and the middle (0,20,640,440) to white through
 * func_80059888, then clears the bytes D_800AF6DC and D_800AF680. */
#include "psx_types.h"

typedef struct {
    s16 x, y, w, h;
} Rect;

extern u16 D_800AF7BC;
extern u16 D_800AF7BE;
extern u16 D_800AF7C0;
extern u8 D_800AF6DC;
extern u8 D_800AF680;
extern void func_800527C4(s32 a, s32 b, s32 c, s32 d, s32 e);
extern void func_80053EEC(s32 a, s32 b, s32 c, s32 d);
extern void func_80059234(s32 a);
extern void func_80053218(void);
extern void func_800147B8(void);
extern void func_80059888(Rect *r, s32 a, s32 b, s32 c);

void func_800CFFEC(void)
{
    Rect r1;
    Rect r2;
    Rect r3;

    D_800AF7BC = 0x280;
    D_800AF7BE = 0x1E0;
    D_800AF7C0 = 1;
    func_800527C4(0x280, 0x1E0, 5, 0, 0);
    func_80053EEC(0, 0, 0, 0);
    func_80059234(1);
    func_80053218();
    func_800147B8();
    r1.x = 0;
    r1.y = 0;
    r1.w = 0x280;
    r1.h = 0x14;
    func_80059888(&r1, 0, 0, 0);
    r2.x = 0;
    r2.y = 0x14;
    r2.w = 0x280;
    r2.h = 0x1B8;
    func_80059888(&r2, 0xFF, 0xFF, 0xFF);
    r3.x = 0;
    r3.y = 0x1CC;
    r3.w = 0x280;
    r3.h = 0x14;
    func_80059888(&r3, 0, 0, 0);
    D_800AF6DC = 0;
    D_800AF680 = 0;
}

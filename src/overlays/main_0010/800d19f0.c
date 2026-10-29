/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame driver for the queued record D_800AE6A8: unless D_80126BA6 is set (or the mode word is 0x3069) it steps the two-phase start sequence D_80114EA8 (func_8001BE20 / func_8001BE30, then func_800CF8E4 when the tag bit 0x1000000 of D_800AE6B0 is clear), then runs the state handler chosen by the record's state byte from a local copy of D_801141C8. Returns 0 when blocked, else the handler's result. */
#include "psx_types.h"

typedef struct Rec800AE6A8 {
    u8 pad00[0x14];
    u8 state;
} Rec800AE6A8;

typedef struct {
    u32 f[3];
} Tbl801141C8;

extern void func_8001BE20(void);
extern s32 func_8001BE30(void);
extern void func_800CF8E4(void);
extern Rec800AE6A8 D_800AE6A8;
extern u32 D_800AE6B0;
extern s16 D_800B9A08;
extern s32 D_80114EA8;
extern u8 D_80126BA6;
extern Tbl801141C8 D_801141C8;

s32 func_800D19F0(void)
{
    Tbl801141C8 t;
    Rec800AE6A8 *r;

    t = D_801141C8;
    r = &D_800AE6A8;
    if (D_80126BA6 != 0) {
        return 0;
    }
    if (D_800B9A08 != 0x3069) {
        switch (D_80114EA8) {
        case 0:
            func_8001BE20();
            D_80114EA8++;
        case 1:
            if (func_8001BE30() == 0) {
                return 0;
            }
            D_80114EA8++;
            if ((D_800AE6B0 & 0x1000000) == 0) {
                func_800CF8E4();
            }
            break;
        case 2:
            break;
        }
    }
    return ((s32 (*)(Rec800AE6A8 *))t.f[r->state])(r);
}

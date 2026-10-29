/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame step of the queued record (argument r is the record, passed through to the callback): clears D_800B9A0E, calls the callback stored at +0x1C of the record named by D_800AE6B0, latches the pending mode (func_800D1D24) and then either plays the transition (func_80011DF4 set: func_80011DCC, func_80011818(0x11)) or picks a sound id for func_80011B7C from the pending-mode checks (func_8002AF08, func_800CFBE8, func_80029264, func_800D1DB0). Finally mirrors the record's top-flag bit 0x1000000 into its byte +0x17. */
#include "psx_types.h"

typedef struct Rec800AE6A8 Rec800AE6A8;

typedef struct {
    u8 pad00[0x1C];
    u32 cb;
} Cb800D1BF8;

struct Rec800AE6A8 {
    u8 pad00[8];
    u32 tag;
    u8 pad0C[0xB];
    u8 b17;
};

extern s32 func_80011DF4(void);
extern void func_80011DCC(void);
extern void func_80011818(s32 a);
extern s32 func_8002AF08(void);
extern s32 func_800CFBE8(void);
extern void func_800CFBBC(void);
extern s32 func_80029264(void);
extern void func_80029254(void);
extern s32 func_800D1DB0(void);
extern void func_80011B7C(s32 k);
extern void func_800D1D24(void);
extern u32 D_800AE6B0;
extern s16 D_800B9A0E;
extern Rec800AE6A8 D_800AE6A8;

void func_800D1BF8(Rec800AE6A8 *r)
{
    Rec800AE6A8 *g;
    s32 a;
    s32 t;

    g = &D_800AE6A8;
    D_800B9A0E = 0;
    ((void (*)(Rec800AE6A8 *))((Cb800D1BF8 *)(D_800AE6B0 & 0x80FFFFFF))->cb)(r);
    func_800D1D24();
    if (func_80011DF4() != 0) {
        func_80011DCC();
        func_80011818(0x11);
    } else {
        if (func_8002AF08() != 0) {
            a = 0xD;
        } else if (func_800CFBE8() != 0) {
            func_800CFBBC();
            a = 0x13;
        } else if (func_80029264() != 0) {
            func_80029254();
            a = 0x12;
        } else {
            t = func_800D1DB0();
            switch (t) {
            case 0:
                a = 6;
                break;
            case 1:
                a = 5;
                break;
            case 2:
            default:
                a = 0x10;
                break;
            }
        }
        func_80011B7C(a);
    }
    if (g->tag & 0x1000000) {
        g->b17 = 1;
    } else {
        g->b17 = 0;
    }
}

/* func_801564B0 [801564B0,801565C0) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x44];
    u32 f44;
} O;

typedef struct {
    u8 p0[0x2E];
    s16 f2E;
} G;

/* callers read v0: yes (caller masks it with 0xFFFF); returns u16 0 or 0x8000. */
extern s32 func_8005C49C(void);
extern s32 func_8002A1B4(void);
extern s32 func_8002A400(void);
extern s32 func_8002A670(void);
extern s32 func_8002A8E0(void);
extern u8 D_80078E78[];

u16 func_801564B0(O *o) {
    G *g;
    s32 t;
    u16 r;

    g = ((G *)&D_80078E78);
    if (o->f44 & 0x200) {
        if (func_8005C49C() & 1) {
            return 0x8000;
        }
    }
    if (func_8002A1B4() != 0 || func_8002A400() != 0 || func_8002A670() != 0 || func_8002A8E0() != 0) {
        if ((func_8005C49C() & 3) == 0) {
            return 0x8000;
        }
    }
    if ((func_8005C49C() & 3) == 0) {
        t = func_8005C49C();
        r = t % 160;
        if (r >= 0x51 && r < g->f2E) {
            return 0x8000;
        }
    }
    return 0;
}

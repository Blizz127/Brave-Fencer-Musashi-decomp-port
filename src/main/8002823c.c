/* func_8002823C: initialise the three 16-byte records at D_800A5E88 and register each. */
#include "psx_types.h"

typedef struct {
    u32 a;
    u32 b;
    u32 c;
    u8 r, g, bl;
    u8 pad;
} Rec;

extern Rec D_800A5E88[];
extern void func_80028620(s32, Rec *);

void func_8002823C(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        D_800A5E88[i].a = 0x800;
        D_800A5E88[i].b = 0x800;
        D_800A5E88[i].c = 0;
        D_800A5E88[i].r = 0x20;
        D_800A5E88[i].g = 0x20;
        D_800A5E88[i].bl = 0x20;
        func_80028620(i, &D_800A5E88[i]);
    }
}


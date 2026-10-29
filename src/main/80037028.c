/* Owner re-derivation from retail assembly (clean input set). */
/* Claims the first free channel of five for a sound id; channel 0 also resets the shared mixer state. */
#include "psx_types.h"
typedef struct {
    u32 f0;
    u32 f4;
    u32 f8;
    u8 fC;
    u8 active;
    u8 kind;
    u8 padF;
} Chan;
typedef struct {
    u32 f00;
    u32 f04;
    u8 pad08[8];
    u32 f10;
    u8 pad14[4];
    s16 f18;
    u8 f1A;
    u8 f1B;
    Chan chan[5];
    u32 f6C;
    u8 pad70[4];
    u32 f74;
} Mixer;
typedef struct { u32 f0; u8 pad[20]; } Info24;
typedef struct { u8 pad0; u8 kind; u8 pad[10]; } Def12;
extern Mixer D_80076228;
extern Info24 D_800A463C[];
extern Def12 D_80064D48[];

void func_80037028(s32 id, u32 val)
{
    s32 i;
    u8 kind;

    for (i = 0; i < 5; i++) {
        if (D_80076228.chan[i].active == 0) {
            break;
        }
    }
    if (i < 5) {
        D_80076228.chan[i].active = 1;
        D_80076228.chan[i].f0 = val;
        D_80076228.chan[i].fC = 0;
        D_80076228.chan[i].f4 = id;
        kind = D_80064D48[id].kind;
        D_80076228.chan[i].kind = kind;
        D_80076228.chan[i].f8 = D_800A463C[kind].f0;
        if (i == 0) {
            D_80076228.f04 = val;
            D_80076228.f10 = D_800A463C[kind].f0;
            D_80076228.f00 = val;
            D_80076228.f18 = 0;
            D_80076228.f1B = 2;
            D_80076228.f1A = 0;
            D_80076228.f6C = 0;
        }
    }
    D_80076228.f74 = 0;
}

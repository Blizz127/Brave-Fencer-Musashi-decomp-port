/* Owner re-derivation from retail assembly (clean input set). */
/* Three-stage state machine; in its last stage it releases the resources of up to five active slots, then resets the stage and returns 1. */
#include "psx_types.h"

typedef struct Rec18 {
    s16 f0;
    s16 f2;
    s16 f4;
    s16 pad6;
    s16 f8;
    u8 padA[6];
    u8 f10;
    u8 pad11[7];
} Rec18;

typedef struct Rec10 {
    s32 f0;
    s32 f4;
    s32 f8;
    u8 fC;
    u8 fD;
    u8 fE;
    u8 padF;
} Rec10;

typedef struct Rec0C {
    u8 f0;
    u8 f1;
    u8 pad2[10];
} Rec0C;

typedef struct Pair {
    s16 a;
    s16 b;
} Pair;

extern s32 D_8007629C;
extern u8 D_8006AEF4;
extern s8 D_80076298;
extern Rec10 D_80076244[];
extern Rec0C D_80064D49[];
extern Rec18 D_800A4640[];
extern Pair D_800C5328[];
extern s32 func_8003C4F0(s32 a);
extern void func_800415A8(s32 a);
extern void func_80031A98(void);

s32 func_8003750C(void) {
    s32 i;
    s32 owner;
    s32 slot;
    s16 cur;

    switch (D_8007629C) {
    case 0:
        if (D_8006AEF4 & 2) {
            return 0;
        }
        D_8006AEF4 |= 2;
        D_8007629C = 1;
        /* fall through */
    case 1:
        if (D_8006AEF4 & 1) {
            if (func_8003C4F0(0) == 0) {
                return 0;
            }
        }
        D_8007629C += 1;
        /* fall through */
    case 2:
        for (i = 0; i < 5; i++) {
            if (D_80076244[i].fD != 0) {
                owner = D_80076244[i].f4;
                slot = D_80076244[i].fE;
                if ((owner & 0x6000) == 0) {
                    cur = D_800A4640[slot].f4;
                    if (cur != 0 && cur != D_80064D49[owner].f1) {
                        D_800C5328[D_800A4640[slot].f8].b = -1;
                        D_800A4640[slot].f4 = 0;
                        D_800A4640[slot].f8 = 0;
                    }
                    if (D_800A4640[slot].f10 == 0) {
                        D_800A4640[slot].f10 = 1;
                        D_800C5328[D_800A4640[slot].f0].a = -1;
                        if (D_800A4640[i].f2 >= 0) {
                            func_800415A8(D_800A4640[i].f2);
                            D_800A4640[i].f2 = -1;
                        }
                        func_80031A98();
                    }
                }
            }
        }
        D_8007629C = 0;
        D_80076298 = 0;
        return 1;
    default:
        return 0;
    }
}

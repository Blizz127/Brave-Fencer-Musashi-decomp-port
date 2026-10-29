/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame update of the scene fade/transition state. When the scene index D_80078EAF changes (and
 * D_801841C0 is clear) the matching table entries are latched (three words of the 12-byte table
 * D_8017ECE8 as bytes, the 9-byte record of D_8017ECB0 into D_80185BC8) and the countdown D_801841B0
 * is armed. The countdown then triggers the step functions (func_8013DBE4 when D_801841D8 is set,
 * func_8013D9B0 otherwise) and eases D_80185B70. Finally D_801841C4 is set for view 0x1010 from the
 * fade phase D_801841B4. */
#include "psx_types.h"

/* 9-byte record, byte aligned (copied with unaligned word moves). */
typedef struct {
    u8 b[9];
} Rec9;

/* 12-byte table entry: three words stored as bytes into three latches. */
typedef struct {
    s32 a;
    s32 b;
    s32 c;
} Ent;

extern s16 D_800B9A08;
extern u8 D_80078EAF;
extern s32 D_801841C0;
extern s32 D_801841B8;
extern s32 D_801841BC;
extern s32 D_801841B0;
extern s32 D_801841B4;
extern s32 D_801841C4;
extern s32 D_801841D0;
extern s32 D_801841D8;
extern s32 D_80185BF0;
extern s32 D_80185B64;
extern s32 D_80185B70;
extern u8 D_80185B31;
extern u8 D_80185C7E;
extern u8 D_80185BEC;
extern Ent D_8017ECE8[];
extern Rec9 D_8017ECB0[];
extern Rec9 D_80185BC8;

extern void func_8013DBE4(s32 n);
extern void func_8013D9B0(s32 n);
extern void func_8013D330(void);
extern void func_8013D178(void);
extern void func_8013CF68(void);

void func_8013CB84(void)
{
    s32 view = D_800B9A08;
    s32 scene = D_80078EAF;
    s32 *q;

    D_80185BF0 = 0;
    if (D_801841C0 == 0 && scene != D_801841B8) {
        D_801841B8 = scene;
        D_80185C7E = D_8017ECE8[scene].a;
        D_80185BEC = D_8017ECE8[scene].b;
        D_80185B31 = D_8017ECE8[scene].c;
        D_80185BC8 = D_8017ECB0[scene];
        D_801841BC = 1;
        D_801841B0 = 1;
        if (D_801841D0 & 2) {
            if (scene == 4) {
                D_80185B64 = 0x100;
                D_80185B70 = 0x80;
            }
            if (scene == 0) {
                D_80185B64 = 0x100;
                D_80185B70 = 0;
            }
        }
    }
    if (D_801841BC != 0) {
        D_801841B0 = D_801841B0 - 1;
        if (D_801841B0 == 0) {
            if (D_801841C0 == 0) {
                if ((D_801841D0 & 2) && scene == 4) {
                    if (D_801841D8 != 0) {
                        func_8013DBE4(6);
                    } else {
                        func_8013D9B0(6);
                    }
                } else {
                    if (D_801841D8 != 0) {
                        func_8013DBE4(scene);
                    } else {
                        func_8013D9B0(scene);
                    }
                }
                func_8013D330();
                func_8013D178();
                func_8013CF68();
                D_801841B0 = 2;
                if (D_801841BC != 0) {
                    if (D_801841D0 & 2) {
                        if (scene == 4) {
                            q = &D_80185B70;
                            *q -= 8;
                            if (*q < 0) {
                                *q = 0;
                            }
                        } else if (scene == 0) {
                            q = &D_80185B70;
                            *q += 0x20;
                            if (*q >= 0x81) {
                                *q = 0x80;
                            }
                        }
                    }
                } else if (D_801841D0 & 2) {
                    if (scene == 4) {
                        D_80185B70 = 0;
                    }
                    if (scene == 0) {
                        D_80185B70 = 0x80;
                    }
                }
            } else {
                if (D_801841D8 != 0) {
                    func_8013DBE4(5);
                } else {
                    func_8013D9B0(5);
                }
                D_801841B0 = 1;
            }
        }
    }
    D_801841C4 = 0;
    if (view == 0x1010) {
        if (scene == 3) {
            if (D_801841BC != 0) {
                D_801841C4 = D_801841B4;
            } else {
                D_801841C4 = 0x1F;
            }
        } else if (scene == 4) {
            if (D_801841BC != 0) {
                D_801841C4 = 0x1F - D_801841B4;
            } else {
                D_801841C4 = 0;
            }
        }
    }
}

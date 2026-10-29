/* Owner re-derivation from retail assembly (clean input set). */
/* Steps the change to table entry D_800C6D34: early-outs, set load (func_800363CC, func_8001A114), then commit and command dispatch. */
#include "psx_types.h"

/* 6-byte records at 0x80063138 (same stride as func_8001B788's view). */
typedef struct {
    s16 set; /* index into D_800AE830; < 0: nothing to load */
    s16 arg;
    s16 cmd; /* command for func_8002D4C8; 0 (first check) / <= 0 (commit): none */
} Entry;

/* 8-byte records at 0x800AE830; id is the value D_800AE708 tracks. */
typedef struct {
    s32 id;
    s32 f4;
} Slot;

extern u32 D_800AE7EC;
extern s32 D_800C6D34;
extern s32 D_800A654C;
extern s32 D_800747F8;
extern s32 D_800C6D10;
extern s32 D_800AE708;
extern s32 D_800AE6F4;
extern s32 D_800AE70C;
extern Entry D_80063138[];
extern Slot D_800AE830[];
extern void func_8002D4C8(u16, u16);
extern s32 func_80034B98(void);
extern void func_80036D58(s16);
extern s32 func_8001B788(s32);
extern s32 func_800363CC(s32, Slot *, s32);
extern s32 func_8001A114(void);
extern void func_8001B710(void);

void func_8001B3C4(void)
{
    s32 result;
    s32 unused_frame[2]; /* retail reserves 8 unused frame bytes */

    result = 0;
    switch (D_800AE7EC) {
    case 0:
        if (D_800C6D34 == D_800A654C) {
            D_800C6D10 = 1;
            return;
        }
        if (D_80063138[D_800C6D34].set == D_800747F8 && D_80063138[D_800C6D34].cmd != 0) {
            func_8002D4C8(D_80063138[D_800C6D34].cmd, 0);
            D_800C6D10 = 1;
            D_800A654C = D_800C6D34;
            return;
        }
        if (func_80034B98() != 0) {
            break;
        }
        if (D_80063138[D_800C6D34].set < 0) {
            func_80036D58(D_80063138[D_800C6D34].arg);
            D_800C6D10 = 1;
            return;
        }
        if (func_8001B788((s16)D_800C6D34) == 0) {
            D_800C6D10 = 1;
            return;
        }
        if (D_800AE708 != 0 && D_800AE830[D_80063138[D_800C6D34].set].id != D_800AE708) {
            break;
        }
        D_800AE7EC++;
        D_800AE708 = D_800AE830[D_80063138[D_800C6D34].set].id;
    case 1:
        result = func_800363CC(D_80063138[D_800C6D34].arg, &D_800AE830[D_80063138[D_800C6D34].set], 0x10);
        if (result == 2) {
            D_800AE7EC++;
            result = 0;
        }
        break;
    case 2:
        D_800AE6F4 = 0;
        D_800AE70C = 0;
        D_800AE7EC = 3;
    case 3:
        if (func_8001A114() != 0) {
            D_800AE7EC = 0;
        }
        break;
    }
    if (result != 0) {
        D_800747F8 = D_80063138[D_800C6D34].set;
        if (D_80063138[D_800C6D34].cmd > 0) {
            func_8002D4C8(D_80063138[D_800C6D34].cmd, 0);
            D_800A654C = D_800C6D34;
            if (D_800C6D34 == 0x3D) {
                func_8001B710();
            }
        }
        D_800AE7EC = 0;
        D_800AE708 = 0;
    }
    D_800C6D10 = result;
}

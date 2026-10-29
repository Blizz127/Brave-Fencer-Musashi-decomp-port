/* Owner re-derivation from retail assembly (clean input set). */
/* Sound driver tick: queued command, master-fade step, channel update and bookkeeping. */
#include "psx_types.h"

extern void func_8002CFE4(void);
extern void func_8002D904(s32);
extern void func_8002EC10(void);
extern void func_80037FC4(void);
extern s16 func_8003836C(s16);
extern s32 func_8003C4F0(s32);
extern void func_8003D25C(s32, s32, u8 *);
extern u16 D_8006A990;
extern u8 D_8006AEF4;
extern s32 D_800A4638;
extern s16 D_800A4E86;
extern s16 D_800A4E8A;
extern u16 D_800A4E8E;
extern u16 D_800A4EE0;
extern u16 D_800A4EE2;
extern u16 D_800A4EE4;
extern u8 D_800A4EE6;
extern u8 D_800A4EFE[];
extern u8 D_800A4F16;
extern u8 D_800A4F17;
extern u8 D_800A4F18;
extern u16 D_800A4F22;

void func_8002CDD8(void)
{
    u16 cmd;
    s32 *tick;
    u8 flags;
    u16 target;
    u16 level;

    cmd = D_800A4F22;
    tick = &D_800A4638;
    *tick += 1;
    if (cmd != 0) {
        D_8006A990 = cmd;
        func_8002D904(cmd);
        D_800A4F22 = 0;
    }
    flags = D_800A4EE6;
    if (flags & 1) {
        D_800A4F16 = 1;
        target = D_800A4EE4;
        level = D_800A4EE0;
        if (target < level) {
            D_800A4EE0 = level - D_800A4EE2;
            if (target < D_800A4EE0) {
                goto skip;
            }
        } else {
            D_800A4EE0 = level + D_800A4EE2;
            if (D_800A4EE0 < target) {
                goto skip;
            }
        }
        D_800A4EE0 = target;
        D_800A4EE6 = flags & 0xFE;
    }
skip:
    func_8003D25C(0, 0x17, D_800A4EFE);
    if (D_800A4E8E & 8) {
        func_80037FC4();
        if (D_800A4E8A != 0) {
            if (--D_800A4E8A == 0) {
                func_8002EC10();
            }
        }
        if (func_8003836C(D_800A4E86) == 0) {
            D_800A4E8E &= 0xFFF7;
        }
    }
    if (D_800A4F17 == 0) {
        D_800A4F18 = 0;
        func_8002CFE4();
    } else {
        D_800A4F18 = 1;
    }
    if ((D_8006AEF4 & 1) && !(D_8006AEF4 & 2) && func_8003C4F0(0) != 0) {
        D_8006AEF4 &= 0xFE;
    }
}

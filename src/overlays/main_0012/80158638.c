/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame check for an actor: when the trigger conditions hold it runs either the full start-up sequence or the fallback update sequence. */
#include "psx_types.h"

extern u8 D_800B9A64;
extern u16 D_80078EB4;
extern u8 D_800B9A17;
extern u8 D_80110EC0[];
extern s32 func_80159464(void *obj);
extern s32 func_8014C98C(void);
extern s32 func_800D0F8C(s32 id);
extern void func_800D0F4C(s32 id);
extern void func_801599A4(void *obj);
extern void func_8016F0E4(void *obj);
extern void func_80159B3C(void *obj);
extern void func_80165718(void *obj);
extern void func_8014BB0C(void);
extern void func_8014BDC8(void);
extern void func_8014B2A8(void);
extern s32 func_8013767C(void *p);
extern void func_80158814(void *obj);

void func_80158638(u8 *obj) {
    u32 w;
    if (D_800B9A64 != 0) {
        return;
    }
    if ((*(u32 *)(obj + 0x44) & 0x10) != 0) {
        return;
    }
    if (*(u16 *)obj == 0x1A) {
        return;
    }
    if (*(u16 *)obj == 0x1E) {
        return;
    }
    w = *(u32 *)(obj + 0x1F8);
    if (w != 0 && (w & 0x8000000) == 0) {
        return;
    }
    if (D_80078EB4 != 0) {
        return;
    }
    if ((s16)func_80159464(obj) <= 0 || (s16)func_80159464(obj) >= 6) {
        if (func_8014C98C() == 0 && func_800D0F8C(0xA) != 0) {
            func_801599A4(obj);
            func_8016F0E4(obj);
            func_80159B3C(obj);
            func_80165718(obj);
            func_800D0F4C(0xA);
            func_8014BB0C();
            func_8014BDC8();
            func_8014B2A8();
            *(s32 *)(obj + 0x198) = func_8013767C(D_80110EC0);
            D_800B9A17 = 1;
            return;
        }
    }
    func_801599A4(obj);
    func_8016F0E4(obj);
    func_80158814(obj);
}

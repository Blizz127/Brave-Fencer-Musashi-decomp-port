/* Owner re-derivation from retail assembly (clean input set). */
/* State step: advances the +0x12 angle, polls the +0x198 handle, and on completion branches on the +0x244 mode. */
#include "psx_types.h"

extern u8 D_80078E50[];
extern s32 D_80078E8C;
extern u8 D_80110C3C[];
extern u8 D_80110D0C[];
extern s32 func_80029504(void);
extern s32 func_80029A94(s32 i);
extern void func_800298BC(u8 *p);
extern void func_8002992C(s32 a);
extern u32 func_8013767C(u8 *p);
extern void func_80139914(u32 h);
extern s32 func_801399F0(u32 h);
extern void func_801458E0(void);
extern void func_801458E8(void);
extern void func_80145A2C(void);
extern void func_80146C98(u8 *o, s32 a);
extern void func_80175454(void);
extern void func_800CF804(void);
extern void func_800CF818(void);

void func_80159120(u8 *o) {
    u8 *p;
    s32 h;
    s32 t;
    func_80029504();
    p = *(u8 **)(o + 0x20);
    *(u16 *)(p + 0x12) = (*(u16 *)(p + 0x12) + 0x16) & 0xFFF;
    h = func_801399F0(*(u32 *)(o + 0x198));
    if (h != 0) {
        func_80139914(*(u32 *)(o + 0x198));
        *(u32 *)(o + 0x198) = 0;
        switch (h) {
        case 1:
        default:
            switch (*(s16 *)(o + 0x244)) {
            case 1:
            default:
                if (func_80029A94(1) != 0) {
                    func_80175454();
                    func_801458E0();
                    t = D_80078E8C;
                    func_800298BC(D_80078E50);
                    func_801458E8();
                    t = t / 2;
                    D_80078E8C = t;
                    goto done;
                } else {
                    *(u32 *)(o + 0x198) = func_8013767C(D_80110D0C);
                    func_80146C98(o, 0xB);
                }
                break;
            case 2:
                if (func_80029A94(0) == 0) {
                    *(u32 *)(o + 0x198) = func_8013767C(D_80110D0C);
                    func_80146C98(o, 0xB);
                } else {
                    func_80175454();
                    func_8002992C(0);
                    func_80145A2C();
                done:
                    func_80146C98(o, 0xC);
                    func_800CF804();
                    func_800CF818();
                }
                break;
            }
            break;
        case 2:
            *(u32 *)(o + 0x198) = func_8013767C(D_80110C3C);
            func_80146C98(o, 9);
            break;
        }
    }
}

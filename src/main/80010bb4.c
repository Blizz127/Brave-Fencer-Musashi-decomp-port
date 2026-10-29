/* Owner re-derivation from retail assembly (clean input set). */
/* Resets the game-loop state flags and reinitialises the field, sound, message and overlay subsystems. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_80010AE0(s32);
extern void func_80011778(void);
extern void func_80011B7C(s32);
extern void func_80015310(void);
extern void func_80018FC8(void);
extern void func_8001903C(void);
extern void func_8001C044(void);
extern void func_80029274(void);
extern void func_8002941C(void);
extern void func_80029690(void);
extern s32 D_80074784;
extern s16 D_801150D4;

void func_80010BB4(void) {
    register u8 *state = D_800AF630;

    state[0xA3E0] = 0;
    state[0xA3E1] = 1;
    state[0xA434] = 0;
    func_8001C044();
    func_80015310();
    func_80029690();
    D_801150D4 = 0;
    D_80074784 = 0;
    func_80018FC8();
    func_8001903C();
    func_80029274();
    func_8002941C();
    func_80010AE0(0x3E0);
    func_80011B7C(0);
    func_80011778();
}

/* SC02/0039 per-frame update uses its own object block. */
#include "psx_types.h"

extern u8 D_80126B58[];
extern u32 D_801A0D90[];
extern u16 D_800B99DA;
extern u8 D_800B9A64;

extern void func_80129CF8(void);
extern void func_8017849C(void);
extern void func_8014FDF4(u8 *obj);
extern s32 func_801505FC(void *self);
extern s32 func_801508B4(void *obj);
extern void func_80165E90(void);
extern void func_801627E8(void);
extern void func_80162B1C(void);
extern void func_80165CA0(void);
extern void func_80129010(void);
extern void func_8013CA14(void);
extern void func_800190AC(void);
extern void func_8012956C(void);
extern void func_8016E95C(void);
extern void func_801754A8(void);
extern void func_8013BC7C(u8 *o);
extern void func_8013BCDC(void);
extern void func_801379FC(void);
extern void func_8001212C(void);

void func_801287B8(void)
{
    func_80129CF8();
    func_8017849C();
    func_8014FDF4(D_80126B58);
    func_801505FC(D_80126B58);
    func_801508B4(D_80126B58);
    func_80165E90();
    func_801627E8();
    func_80162B1C();
    func_80165CA0();
    func_80129010();
    func_8013CA14();
    func_800190AC();
    func_8012956C();
    func_8016E95C();
    func_801754A8();
    if (D_801A0D90[0] == 0) {
        func_8013BC7C((u8 *)D_801A0D90);
    }
    func_8013BCDC();
    func_801379FC();
    D_800B99DA++;
    if (D_800B9A64 != 0) {
        func_8001212C();
    }
}

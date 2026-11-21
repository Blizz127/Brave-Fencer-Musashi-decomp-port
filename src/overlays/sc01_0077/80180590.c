/* Owner-disc reconstruction. Allocate and initialize the standing actor. */
#include "psx_types.h"

extern void *func_8012C1B8(void);
extern s32 func_8012CAE4(u8 *actor);
extern void func_8001C214(void *model, s32 *list);
extern void func_8001D0E8(void *model, s16 width, s16 height);
extern s32 func_8012A828(u8 *actor, u32 configuration);
extern void func_8012B2CC(char *actor);

extern s32 D_801D15C4[], D_801D1CBC[];
extern u8 D_8018A8A4[], D_8018A8B4[], D_8018A8C4[], D_80186E48[];

void func_80180590(u8 *actor)
{
    u8 *model = func_8012C1B8();
    s32 *list;
    s16 width, height;

    *(u32 *)(actor + 0x20) = (u32)(unsigned long)model;
    if (!model) {
        func_8012CAE4(actor);
        return;
    }
    list = D_801D15C4;
    if (actor[0x70] == 4)
        list = D_801D1CBC;
    func_8001C214(model, list);
    if (actor[0x70] != 4) {
        model = (u8 *)(unsigned long)*(u32 *)(actor + 0x20);
        width = 0x168;
        height = 0x140;
        *(u32 *)(actor + 0x58) = (u32)(unsigned long)D_8018A8A4 | 0x40000000u;
    } else {
        model = (u8 *)(unsigned long)*(u32 *)(actor + 0x20);
        width = 0x7fff;
        height = 0x7fff;
        *(u32 *)(actor + 0x58) = (u32)(unsigned long)D_8018A8B4 | 0x40000000u;
    }
    func_8001D0E8(model, width, height);
    *(u32 *)(actor + 0x78) = (u32)(unsigned long)D_8018A8C4;
    actor[0x75] = 4;
    func_8012A828(actor, (u32)(unsigned long)D_80186E48);
    *(u16 *)(actor + 2) = 1;
    if (actor[0x70] == 4)
        *(u16 *)(actor + 0x5c) |= 0x8000;
    else
        *(u16 *)(actor + 0x5c) = 0xc800;
    *(u16 *)((u8 *)(unsigned long)*(u32 *)(actor + 0x20) + 0x10) = 0;
    func_8012B2CC((char *)actor);
}

/* Bind the child's selected model and retain its owner's current allocation serial. */
#include "psx_types.h"

extern void *func_8012C1B8(void);
extern s32 func_8012CAE4(u8 *actor);
extern void func_8001C214(void *model,s32 *list);
extern s32 *D_8018AFA8[];

void func_80184CCC(u8 *actor)
{
    void *model = func_8012C1B8();
    u8 *owner;
    u16 serial;
    *(u32 *)(actor + 0x20) = (u32)(unsigned long)model;
    if (!model) {
        func_8012CAE4(actor);
    } else {
        func_8001C214(model, D_8018AFA8[*(s16 *)(actor + 0x70)]);
        owner = (u8 *)(unsigned long)*(u32 *)(actor + 0x64);
        serial = *(u16 *)(owner + 0x36);
        *(u16 *)(actor + 2) = 1;
        *(u16 *)(actor + 0xfe) = 0x7fff;
        *(u16 *)(actor + 0xfc) = serial;
    }
}

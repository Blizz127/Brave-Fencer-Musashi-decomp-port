/* The first state acquires a model before advancing the actor. */
#include "psx_types.h"

extern void *func_8012C1B8(void);
extern s32 func_8012CAE4(u8 *actor);
extern void func_8001CA88(void *model, s32 configuration);
extern s32 func_80143994(u8 *actor, s16 argument);
extern u8 D_8018AFC0[];

void func_80184944(u8 *actor)
{
    void *model = func_8012C1B8();
    *(u32 *)(actor + 0x20) = (u32)(unsigned long)model;
    if (model == 0) {
        func_8012CAE4(actor);
    } else {
        *(u16 *)(actor + 2) = 1;
        func_8001CA88((void *)(unsigned long)*(u32 *)(actor + 0x20),
                      (s32)(unsigned long)D_8018AFC0);
        func_80143994(actor, 0x1000);
    }
}

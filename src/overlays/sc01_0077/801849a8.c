/* Start the actor's 80-update state with its saved model angle. */
#include "psx_types.h"

extern void func_8012B2CC(char *actor);
extern void func_8012B200(u8 *actor);
extern void func_8012B178(u8 *actor, s32 local_z);

void func_801849A8(u8 *actor)
{
    u8 *model = (u8 *)(unsigned long)*(u32 *)(actor + 0x20);
    u16 angle = *(u16 *)(actor + 0xfc);

    *(u16 *)(actor + 2) = 2;
    *(s32 *)(actor + 0x1c) = 80;
    *(u16 *)(model + 0x12) = angle;
    func_8012B2CC((char *)actor);
    func_8012B200(actor);
    func_8012B178(actor, -0x100000);
}

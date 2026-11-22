/* Wait for the player to face the camera position before starting the event. */
#include "mine_award_middle.h"

void func_80173CB4(u8 *actor)
{
    s16 position[3];
    position[0] = (s16)CAMERA_WORD(0x3c);
    position[1] = (s16)CAMERA_WORD(0x40);
    position[2] = (s16)CAMERA_WORD(0x44);
    if ((s16)func_80171AB0(actor, position))
        return;

    func_8014706C(actor);
    switch (SETUP_MODE()) {
    case 0:
        func_80154274(actor, D_800D5DA8);
        WRITE_COUNT(actor, 32);
        func_80147324(2115);
        break;
    case 1:
        func_80154274(actor, D_800D5DA8);
        func_8016BADC(actor, -64, 64);
        WRITE_COUNT(actor, 128);
        func_80147324(1702);
        break;
    case 2:
        func_80154274(actor, D_800D5DA8);
        func_8016BADC(actor, -64, 64);
        WRITE_COUNT(actor, 128);
        func_80147324(1702);
        break;
    case 3:
        func_80154274(actor, D_800D5DB8);
        func_801553A8(actor);
        func_8016BADC(actor, -64, 64);
        WRITE_COUNT(actor, 128);
        func_80147324(1702);
        break;
    }
    if (D_8012750C)
        WRITE_HANDLE(actor, func_8013767C(D_8012750C));
    else
        WRITE_HANDLE(actor, 0);
    func_80171A1C(actor);
}

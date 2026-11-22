/* Clean-room reconstruction from the owner's SC01/0077 overlay. */
#include "psx_types.h"

extern s32 D_801270CC[];
extern s32 D_801270D4;
extern u16 D_80126B66;
extern s16 D_8018A8FC[];
extern s32 D_8018A8F8;
extern void func_8017F5BC(u8 *actor);
extern void func_8002D4C8(u16 command, u16 argument);

void func_8017F460(u8 *actor)
{
    s32 distance;

    if (actor[0x70] == 4) {
        if (D_801270CC[0] == 0 && (*(u16 *)(actor + 0x5c) & 1)) {
            if (!D_801270D4) {
                D_801270D4 = 1;
            }
            D_801270CC[0] = 3;
            *(u16 *)(actor + 0x5c) = 0;
        }
        if (D_801270CC[0] >= 2) {
            if ((*(u16 *)(actor + 0x5c) & 1) && !D_801270D4) {
                D_801270D4 = 1;
            }
            if (D_801270D4 >= D_8018A8FC[*(s16 *)(actor + 0x70) >> 8]) {
                func_8017F5BC(actor);
                func_8002D4C8(0x495, 0);
            }
        }
    } else {
        distance = (s16)D_80126B66 - *(s16 *)(actor + 0x0e);
        if (distance < 0) {
            distance = -distance;
        }
        if (distance < D_8018A8F8) {
            func_8017F5BC(actor);
        }
    }
}

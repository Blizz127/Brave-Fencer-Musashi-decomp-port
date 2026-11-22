#include "tower_handover.h"

u32 func_80175414(void)
{
    MineFrameMode *mode = &D_8011F7A8;
    if (mode->mode == 1) {
        TOWER_INTRO_RESULT(result) = 10;
        *(volatile u8 *)&mode->mode = 0;
        HANDOVER_MODE_BYTE5 = result;
        result = 55;
        HANDOVER_MODE_BYTE1 = 0;
        HANDOVER_MODE_BYTE6 = result;
        return result;
    }
    return 10;
}

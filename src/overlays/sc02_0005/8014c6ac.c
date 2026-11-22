#include "tower_handover.h"

u32 func_8014C6AC(void)
{
    TOWER_INTRO_RESULT(result);
    result = 1;
    HANDOVER_FLAG = result;
    return result;
}

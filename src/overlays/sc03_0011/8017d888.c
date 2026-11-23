#include "plant_exit.h"

int func_8017D888(struct PlantExit *door)
{
    if (--door->timer == ~0u) {
        func_80029124(144, 1);
        func_8002D4C8(28, 0);
        func_8001BFD0();
        func_8002D4C8(29, 0);
        func_800D0C48(1);
        func_800D1E28();
        ++door->phase;
    }
    return 0;
}

#include "plant_exit.h"

int func_8017D7CC(struct PlantExit *door)
{
    func_8017DBD8();
    door->timer = 64;
    ++door->phase;
    return 0;
}

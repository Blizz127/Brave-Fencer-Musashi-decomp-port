#include "forest_tutorial_soldier.h"

extern void func_8017F240(void);

void func_8017E460(ForestTutorialScript *script)
{
    if (--script->countdown == (u32)-1) {
        func_8017F240();
        script->countdown = 64;
        ++script->stage;
    }
}

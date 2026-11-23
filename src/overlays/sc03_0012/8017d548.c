#include "repair_exit.h"

int func_8017D548(struct ExitTransition *transition)
{
    if (func_80029504() >= 980) {
        func_80029514(1000);
    } else {
        func_80029514(130);
        func_80029124(238, 1);
    }
    func_800291A0(205, (unsigned char)(func_800291B4(205) + 1));
    transition->countdown = 10;
    ++transition->phase;
    return 0;
}

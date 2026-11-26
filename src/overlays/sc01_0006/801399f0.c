#include "file_choice_window.h"

s32 func_801399F0(const struct FileChoiceWindow *window)
{
    if (!window->active || window->state == 0 || window->state == 14) {
        return -1;
    }
    if (window->state == 13) {
        return window->answer;
    }
    return 0;
}

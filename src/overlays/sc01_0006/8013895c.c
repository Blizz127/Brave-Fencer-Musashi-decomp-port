#include "file_choice_window.h"

extern struct FileChoiceWindow *D_80127524;
extern s16 func_80138DB8(struct FileChoiceWindow *window, u8 port, u16 buttons);
extern void func_80138B88(struct FileChoiceWindow *window);
extern void func_8002D4C8(u16 command, u16 parameter);

void func_8013895C(struct FileChoiceWindow *window)
{
    if (D_80127524 != window && !(window->flags & 0x2000)) {
        return;
    }

    if (func_80138DB8(window, 0, 0x40)) {
        window->answer++;
        func_80138B88(window);
        func_8002D4C8(0x630, 0);
    } else if (func_80138DB8(window, 0, 0x1000)) {
        if (--window->answer < 0) {
            window->answer = window->row_count - 1;
        }
        func_8002D4C8(0x62F, 0);
    } else if (func_80138DB8(window, 0, 0x4000)) {
        if (++window->answer >= window->row_count) {
            window->answer = 0;
        }
        func_8002D4C8(0x62F, 0);
    } else if ((window->flags & 2) && func_80138DB8(window, 0, 0x30)) {
        window->answer = window->row_count - 1;
        func_8002D4C8(0x62F, 0);
    }
}

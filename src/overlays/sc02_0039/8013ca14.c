/* Advance the mine's signed value and duration script once per scene update. */
#include "mine_step_script.h"

#ifdef __mips__
typedef struct {
    s8 value;
    u8 time;
} MineScriptStep;
#endif

void func_8013CA14(void)
{
    u32 *script = &D_8019FA30;

    if (*script != 0) {
        if (D_8019FA35 == 0) {
            u8 index = D_8019FA34;
#ifdef __mips__
            s32 value = ((MineScriptStep *)D_8019FA30)[index].value;
#else
            u32 entry = D_8019FA30 + (u32)index * 2;
            u8 byte = bfm_mine_script_read(entry);
            s32 value = byte < 128 ? (s32)byte : (s32)byte - 256;
#endif
            u8 duration;

            D_801269D4 = value;
            D_801269C8 = value;
#ifdef __mips__
            duration = ((MineScriptStep *)D_8019FA30)[index].time;
#else
            duration = bfm_mine_script_read(entry + 1);
#endif
            D_8019FA34 = index + 1;
            D_8019FA35 = duration;
            if (duration == 0) {
                D_801269D4 = 0;
                D_801269C8 = 0;
                D_8019FA34 = 0;
                *script = 0;
            }
        } else {
            D_8019FA35 = D_8019FA35 - 1;
        }
    }
}

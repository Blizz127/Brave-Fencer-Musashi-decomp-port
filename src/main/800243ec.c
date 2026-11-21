#include "model_draw.h"

extern s32 D_80078D88;
extern s32 D_800A2B78;

void func_800243EC(DrawArg *arg, OtRec *ot, s32 mode)
{
    s32 flags = (s32)arg->attr;
    D_80078D88 = flags;
    D_800A2B78 = (flags >> 28) & 7;
    if (flags & 0x40) {
        func_80026D64(arg, ot, mode);
        return;
    }
    func_80024448(arg, ot, mode);
}

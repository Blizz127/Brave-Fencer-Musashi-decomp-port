#include "psx_types.h"

typedef struct {
    s8 offset;
    u8 duration;
} CameraShakeStep;

extern const CameraShakeStep *D_801C4C60;
extern u8 D_801C4C64;
extern u8 D_801C4C65;
extern const u8 D_80062BC0[];
extern s32 func_80019198(void);
extern void func_80019064(s32 sequence);

/* Start the supplied shake pattern at its first step. */
void func_8013C9C4(const CameraShakeStep *pattern)
{
    D_801C4C60 = pattern;
    D_801C4C64 = 0;
    D_801C4C65 = 0;
    if (func_80019198() == 0) {
        func_80019064((s32) D_80062BC0);
    }
}

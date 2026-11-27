#include "psx_types.h"

typedef struct SkullpionCore SkullpionCore;

typedef struct {
    u8 unused00[0x6c];
    SkullpionCore *core;
} Skullpion;

struct SkullpionCore {
    u8 unused00[0x1c];
    s32 recovery_timer;
    u8 unused20[0x44];
    Skullpion *boss;
};

extern void func_8017F93C(void);
extern void func_8012C218(SkullpionCore *core);

/* Retire the exposed core once the nonfatal hit recovery has elapsed. */
void func_80187AB0(SkullpionCore *core)
{
    if (core->recovery_timer) {
        core->recovery_timer--;
    } else {
        func_8017F93C();
        core->boss->core = 0;
        func_8012C218(core);
    }
}

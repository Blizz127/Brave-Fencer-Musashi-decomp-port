#include "psx_types.h"

typedef struct {
    u8 pad00[2];
    u16 phase;
} GuardInteractionState;

typedef struct {
    u8 pad00[0x64];
    GuardInteractionState *state;
} GuardInteraction;

extern s32 func_8012BD14(GuardInteraction *interaction);
extern void func_80178BF8(void);
/* This routine hands the callback to the caller without invoking it here. */
typedef void (*GuardInteractionCallback)();
extern void func_80172710();

GuardInteractionCallback func_8017FCD0(GuardInteraction *interaction)
{
    if (interaction->state->phase != 1) {
        return 0;
    }
    if (func_8012BD14(interaction) >= 0x4000) {
        return 0;
    }
    interaction->state->phase++;
    func_80178BF8();
    return func_80172710;
}

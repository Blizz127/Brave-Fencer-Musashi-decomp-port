#include "psx_types.h"

typedef struct {
    u8 reserved[2];
    u16 phase;
} GuardActor;

extern s32 func_80178970(GuardActor *actor);
extern void func_80178D18(GuardActor *actor);

void func_8017FC90(GuardActor *actor)
{
    if (func_80178970(actor)) {
        func_80178D18(actor);
        actor->phase = 1;
    }
}

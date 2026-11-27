/* The last scroll effect must finish before the long presentation pause. */
#include "scroll_actor.h"

extern struct { short element; } D_80126B3C;
extern int func_801399F0(unsigned int effect);
extern void func_80139914(unsigned int effect);
extern void func_8017FE64(int element);

void func_80180258(ScrollActor *actor)
{
    if (func_801399F0(actor->effect_handle)) {
        func_80139914(actor->effect_handle);
        actor->effect_handle = 0;
        actor->wait = 96;
        func_8017FE64(D_80126B3C.element);
        func_80171A1C(actor);
    }
}

/* Select the final acquisition sequence when the final delay expires. */
#include "scroll_actor.h"

extern unsigned char D_800B9A11;
extern const unsigned int D_80194D94[];
extern void func_80154274(ScrollActor *actor, const unsigned int *sequence);
extern void func_8013C964(void);
extern void func_80129428(void);

void func_801802C0(ScrollActor *actor)
{
    if (--actor->wait == -1) {
        func_80154274(actor, D_80194D94);
        func_8013C964();
        D_800B9A11 = 0;
        func_80129428();
        func_80171A1C(actor);
    }
}

/* Select the first scroll pose after the short delay. */
#include "scroll_actor.h"

extern unsigned char D_800B9A11;
extern const unsigned int D_80194CF0[];
extern void func_8014706C(void);
extern void func_80154274(ScrollActor *actor, const unsigned int *sequence);
extern void func_8013C938(void);
extern void func_8012943C(void);

void func_801800E8(ScrollActor *actor)
{
    if (--actor->wait == -1) {
        func_8014706C();
        func_80154274(actor, D_80194CF0);
        func_8013C938();
        D_800B9A11 = 192;
        func_8012943C();
        func_80171A1C(actor);
    }
}

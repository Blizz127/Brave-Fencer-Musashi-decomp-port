/* A zero final vector halfword skips the movement service. */
#include "mine_award_early.h"

void func_80173C64(u8 *actor)
{
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    extern s16 D_80126B30[4];
    if (D_80126B30[3] && !func_80171CC4(actor, (const u8 *)D_80126B30))
        return;
#else
    extern MineAwardVector D_80126B30;
    if ((D_80126B30.bytes[6] || D_80126B30.bytes[7]) &&
        !func_80171CC4(actor, D_80126B30.bytes))
        return;
#endif
    func_80171A1C(actor);
}

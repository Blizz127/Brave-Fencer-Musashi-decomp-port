/* Request the mine event's fixed 25-point capacity increase. */
#include "mine_stat_event.h"

extern void func_8014BCEC(void *actor, s32 increment);

void func_80174188(void *actor)
{
    func_8014BCEC(actor, 25);
}

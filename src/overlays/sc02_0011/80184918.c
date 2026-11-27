/* Start a score transition and remember the new track until its delay expires. */
#include "rootrick_event.h"

extern void func_800D0C48(int mode);

void func_80184918(RootrickEvent *actor, int score)
{
    func_800D0C48(1);
    actor->score_phase = 1;
    actor->score_delay = 30;
    actor->next_score = score;
}

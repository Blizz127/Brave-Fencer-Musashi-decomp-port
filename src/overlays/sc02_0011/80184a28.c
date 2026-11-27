/* Script wait for the actor timer to run down. */
#include "rootrick_event.h"

int func_80184A28(const RootrickEvent *actor)
{
    return actor->wait == 0;
}

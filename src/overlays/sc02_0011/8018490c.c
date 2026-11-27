/* Keep the selected script together with its race outcome. */
#include "rootrick_event.h"

void func_8018490C(RootrickEvent *actor, unsigned int script, short event_id)
{
    actor->event_script = script;
    actor->event_id = event_id;
}

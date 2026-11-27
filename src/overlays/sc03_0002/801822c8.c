#include "leno_command.h"

void func_801822C8(void)
{
    struct LenoActor *leno = func_8012E544(LENO_ACTOR_CLASS);

    if (leno)
        leno->command = LENO_RELEASE;
}

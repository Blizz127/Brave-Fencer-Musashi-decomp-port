#include "bp_debit.h"

void func_8014BD60(struct Action21Actor *actor, int amount)
{
    unsigned short current = D_80078EB8;

    (void)actor;
    if (current < amount) {
        D_80078EB8 = 0;
    } else {
        D_80078EB8 = (unsigned short)((unsigned int)current - (unsigned int)amount);
    }
}

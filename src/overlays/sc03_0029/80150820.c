#include "bp_credit.h"

void func_80150820(struct Action21Actor *actor, int amount)
{
    if (amount != 0) {
        func_8014BD24(actor, amount);
        func_8014ACE8(actor, 4, amount);
    }
}

#include "bp_credit.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define BP_FIELD(field) (field)
#else
/* Both writes matter when the wrapped sum exceeds the cap. */
#define BP_FIELD(field) (*(volatile unsigned short *)&(field))
#endif

void func_8014BD24(struct Action21Actor *actor, int amount)
{
    unsigned short current = BP_FIELD(D_80078EB8);
    unsigned short maximum = BP_FIELD(D_80078EB6);
    unsigned short credited = (unsigned short)((unsigned int)current + (unsigned int)amount);

    (void)actor;
    BP_FIELD(D_80078EB8) = credited;
    if (credited > maximum) {
        BP_FIELD(D_80078EB8) = maximum;
    }
}

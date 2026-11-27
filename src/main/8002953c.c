#include "psx_types.h"

extern u32 D_80078E84;

/* The resident rescue total is shared by the different field captives. */
u32 func_8002953C(void)
{
    return D_80078E84;
}

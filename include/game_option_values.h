#ifndef GAME_OPTION_VALUES_H
#define GAME_OPTION_VALUES_H

#include "psx_types.h"

/* Both option readers return a zero-extended byte in a full return register. */
s32 func_80028FBC(void);
s32 func_80029000(void);

#endif

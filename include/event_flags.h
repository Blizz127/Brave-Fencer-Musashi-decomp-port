#ifndef BFM_EVENT_FLAGS_H
#define BFM_EVENT_FLAGS_H

#include "psx_types.h"

/* Set or clear one bit and return the complete updated byte, zero-extended. */
u32 func_80029124(u32 bit, u32 set);

#endif

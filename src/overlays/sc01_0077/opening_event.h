#ifndef SC01_0077_OPENING_EVENT_H
#define SC01_0077_OPENING_EVENT_H

#include "player_direction.h"

/* The native dispatcher passes a byte pointer to a naturally aligned ScenePlayer.
 * Historical GCC needs the typed parameter to reproduce the original register use. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define HISTORICAL_OPENING_ABI 1
#define RECORD_ADDRESS(label) ((u32)D_##label)
#define OPENING_OBJECT(type, address) ((type *)(address))
#else
#define HISTORICAL_OPENING_ABI 0
#define RECORD_ADDRESS(label) 0x##label##U
extern u8 *bfm_opening_guest_ptr(u32 address);
#define OPENING_OBJECT(type, address) ((type *)bfm_opening_guest_ptr(address))
#endif

#endif

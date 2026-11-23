#ifndef MUSASHI_TWINPEAK_CONTACT_STATES_H
#define MUSASHI_TWINPEAK_CONTACT_STATES_H

#include "action22_poll.h"

#if !defined(__mips__) || defined(__LP64__)
#define D_800AE6B0 bfm_twinpeak_contact_status
#define D_8018A208 bfm_twinpeak_contact_resource_words
/* The host actor stores a 32-bit guest handle at offset 0x198. */
#define twinpeak_contact_handle bfm_twinpeak_open_contact
extern TwinpeakPollHandle twinpeak_contact_handle(FusionWord resource);
#define func_8014BFF4 bfm_twinpeak_set_contact_byte
#define func_8016007C bfm_twinpeak_clear_contact
#define func_8015FCC8 bfm_twinpeak_prepare_contact
#define func_8015FD74 bfm_twinpeak_open_contact_phase
#else
#include "handle.h"
#define twinpeak_contact_handle func_8013767C
#endif

extern FusionWord D_800AE6B0;
extern FusionWord D_8018A208[];
extern void func_8014BFF4(Action21Actor *actor, unsigned int value);
extern void func_8016007C(Action21Actor *actor);
extern void func_8015FCC8(Action21Actor *actor);
extern void func_8015FD74(Action21Actor *actor);

#endif

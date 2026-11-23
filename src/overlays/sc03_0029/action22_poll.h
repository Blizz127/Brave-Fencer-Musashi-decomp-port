#ifndef MUSASHI_TWINPEAK_ACTION22_POLL_H
#define MUSASHI_TWINPEAK_ACTION22_POLL_H

#include "fusion_queue_game.h"

#if defined(__mips__) && !defined(__LP64__)
typedef void *TwinpeakPollHandle;
#else
typedef FusionWord TwinpeakPollHandle;
#define func_8015FDCC bfm_twinpeak_poll_action22
#define func_801399F0 bfm_twinpeak_poll_handle
#define func_80147324 bfm_twinpeak_request_effect
#define func_80139914 bfm_twinpeak_close_handle
#define func_80165700 bfm_twinpeak_check_contact
#define func_80146CA0 bfm_twinpeak_advance_actor
#endif

typedef struct TwinpeakPollActor {
    Action21Actor base;
    unsigned char unknownBC[2];
    unsigned char contact_flag_be;
    unsigned char contact_flag_bf;
    unsigned char unknownC0[0xc4];
    FusionWord selected_contact;
    unsigned char unknown188[15];
    unsigned char contact_flag_197;
    TwinpeakPollHandle handle;
    unsigned char unknown19C[4];
    unsigned char contact;
} TwinpeakPollActor;

extern int func_80028DE0(void);
extern int func_801399F0(TwinpeakPollHandle);
extern void func_80139914(TwinpeakPollHandle);
extern void func_80147324(int);
extern unsigned int func_80165700(Action21Actor *, unsigned int);
extern void func_80146CA0(Action21Actor *);
extern void func_8015FDCC(Action21Actor *);

#endif

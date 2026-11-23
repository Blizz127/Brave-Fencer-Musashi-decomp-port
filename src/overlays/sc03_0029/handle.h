#ifndef TWINPEAK_TUTORIAL_HANDLE_H
#define TWINPEAK_TUTORIAL_HANDLE_H

#if !defined(__mips__) || defined(__LP64__)
#define D_801269F0 bfm_twinpeak_tutorial_slots
#define D_80127524 bfm_twinpeak_tutorial_active
#define D_80127528 bfm_twinpeak_tutorial_previous
#define func_8013767C bfm_twinpeak_tutorial_open
#define func_801377B4 bfm_twinpeak_tutorial_initialize
#define func_80137840 bfm_twinpeak_tutorial_defaults
#define func_801399A8 bfm_twinpeak_tutorial_find_slot
#endif

typedef struct TutorialHandleSlot {
    unsigned int descriptor;
    short state;
    unsigned short reserved06;
    int flags;
    unsigned char reserved0c[14];
    short active_state;
    unsigned char reserved1c[48];
} TutorialHandleSlot;

extern TutorialHandleSlot D_801269F0[3];
extern TutorialHandleSlot *D_80127524;
extern TutorialHandleSlot *D_80127528;

TutorialHandleSlot *func_8013767C(unsigned int descriptor);
void func_801377B4(unsigned int descriptor, int flags, TutorialHandleSlot *slot);
void func_80137840(TutorialHandleSlot *slot);
TutorialHandleSlot *func_801399A8(void);

#endif

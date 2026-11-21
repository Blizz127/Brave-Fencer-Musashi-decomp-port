#ifndef BFM_SOUND_VOICE_RELEASE_H
#define BFM_SOUND_VOICE_RELEASE_H

#include "psx_types.h"

typedef void (*VoiceCallback)(u8 slot, u32 data);

typedef struct {
    u8 pad0[0x14];
    u32 stage;
    u8 pad18[0x08];
    s32 base;
    s32 current;
    s32 target;
    s32 step;
    u8 pad30[0x07];
    u8 flags;
    u8 done;
} PitchSlide;

typedef union {
    PitchSlide slide;
    struct {
        u32 pending;
        u8 reserved04[6];
        u16 number;
    } channel;
} VoicePrefix;

typedef struct {
    VoicePrefix prefix;
    u8 reserved3c[4];
    VoiceCallback callback;
    u32 callback_data;
    u8 reserved48[6];
    u8 state;
    u8 reserved4f;
    u8 release;
    u8 slot;
} Voice;

typedef struct {
    u32 flags;
    u8 reserved04[61];
    u8 release;
    u8 reserved42[6];
} VoiceSlot;

/* The historical compiler addresses this shared byte through its containing storage. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern u8 D_800A4F17[];
#define BUSY_BYTE D_800A4F17[0]
#else
extern u8 D_800A4F17;
#define BUSY_BYTE D_800A4F17
#endif
extern s32 D_800C7D20;
extern s32 D_80073140[];
extern VoiceSlot D_800A4C2C[];
extern void func_8002EFF8(s32 mode, s32 bits);
extern void func_80031988(PitchSlide *slide);
void func_80030D80(Voice *voice, s32 release);

#endif

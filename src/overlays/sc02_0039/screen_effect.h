#ifndef BFM_SC02_SCREEN_EFFECT_H
#define BFM_SC02_SCREEN_EFFECT_H
#include "psx_types.h"

#if defined(__mips__) && !defined(__LP64__)
#define BFM_SCREEN_DIRECT 1
typedef s16 ScreenSignedHalf;
typedef u16 ScreenHalf;
typedef u32 ScreenAddress;
#else
#define BFM_SCREEN_DIRECT 0
typedef struct { u8 le[2]; } ScreenHalf;
typedef ScreenHalf ScreenSignedHalf;
typedef struct { u8 le[4]; } ScreenAddress;
#endif

typedef struct {
    ScreenSignedHalf x, y;
} ScreenPair;

typedef struct ScreenEffect {
    u8 reserved00[0x18];
    ScreenHalf mode, parameter;
    u8 reserved1c[4];
    u8 wide, reserved21, flags, reserved23;
    ScreenHalf center_x, center_y;
    ScreenPair factors;
    u8 reserved2c[8];
    ScreenSignedHalf extent_x, extent_y;
} ScreenEffect;

typedef struct {
    u8 reserved00[4];
    ScreenAddress effect_address;
    ScreenPair scaled;
    ScreenSignedHalf level, target;
} ScreenWork;

void func_8013A530(ScreenWork *work);
#if !BFM_SCREEN_DIRECT
/* Return a readable 56-byte effect view for this guest address, or NULL. */
ScreenEffect *bfm_screen_effect_resolve(u32 address);
#endif

/* Observer interfaces specific to this caller. The four leading values
 * and five following values are the A0-A3 and outgoing stack words. */
void bfm_screen_observe_15f04(s32 a0, s32 a1, s32 a2, s32 a3,
                              u32 sp10, u32 sp14, u32 sp18, u32 sp1c, u32 sp20);
void bfm_screen_observe_15d4c(s32 a0, s32 a1, s32 a2, s32 a3,
                              u32 sp10, u32 sp14, u32 sp18, u32 sp1c, u32 sp20);
/* A2 and A3 are live on entry to the opaque service. The observer records
 * this call site without defining the service's argument or return types. */
void bfm_screen_observe_3aa24(ScreenEffect *a0, ScreenWork *a1,
                              ScreenWork *a2, u32 a3);
#endif

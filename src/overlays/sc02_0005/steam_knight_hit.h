#ifndef BFM_STEAM_KNIGHT_HIT_H
#define BFM_STEAM_KNIGHT_HIT_H

#include "psx_types.h"

typedef struct {
    u8 bytes[8];
} SteamKnightHitPosition;

/* Fields touched when a Steam Knight attachment consumes a pending hit. */
typedef struct {
    u16 active;
    u16 state;
    u8 pad004[6];
    s16 position_y;
    u8 pad00c[2];
    s16 field_0e;
    s32 motion_x;
    s32 motion_y;
    s32 motion_z;
    s32 timer;
    u32 model_address;
    u8 pad024[52];
    u32 display_record;
    u16 flags;
    u16 hit_kind;
    s16 damage;
    u16 contact_parameter;
    u32 body_address;
    u8 pad068[8];
    s16 variant;
    u8 pad072[4];
    s16 hp;
    u8 pad078[4];
    SteamKnightHitPosition hit_position;
    u8 pad084[42];
    s16 field_ae;
    u8 pad0b0[17];
    u8 reaction;
    u8 pad0C2[6];
    u8 contact_tag;
    u8 contact_frame;
    u8 pad0CA[2];
    u32 linked_address;
    u32 secondary_address;
    u32 core_address;
    u8 pad0d8[4];
    s32 field_dc;
    s16 field_e0;
    u8 pad0e2[6];
    u32 phase_flags;
    u8 pad0ec[16];
    u16 field_fc;
    u16 break_count;
    s16 break_delay;
    u8 pad102[2];
    s16 break_cause;
    u8 pad106[6];
} SteamKnightPart;

SteamKnightPart *func_8014C4AC(SteamKnightPart *part, int kind, int amount,
                               const void *position, u16 parameter);
void func_801853EC(SteamKnightPart *part);
void func_8016AA50(SteamKnightPart *part, int amount);
void func_80187280(SteamKnightPart *part, u16 key);
void func_80185060(SteamKnightPart *part, int mode);
void func_80186B08(SteamKnightPart *part);
void func_80186A04(SteamKnightPart *part);
void func_8002D4C8(u16 command, u16 argument);

#endif

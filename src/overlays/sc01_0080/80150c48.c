/* SPDX-License-Identifier: MIT */
#include "psx_types.h"

struct TowerTransfer;

typedef struct {
    u8 reserved00[0xdc];
    struct TowerTransfer *destination;
    u8 reservede0[0x1c];
    u16 entry;
} TowerExit;

typedef struct {
    u8 reserved00[0x1a4];
    TowerExit *volatile contact;
} TowerPlayer;

extern int func_80150480(TowerPlayer *);
extern struct TowerTransfer *volatile D_800AE6B0;
extern volatile u16 D_800AE6DC;

/* A transfer floor takes its destination and entry from the exit underfoot. */
void func_80150C48(TowerPlayer *player)
{
    if (func_80150480(player)) {
        D_800AE6B0 = player->contact->destination;
        D_800AE6DC = player->contact->entry;
    }
}

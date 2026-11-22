#ifndef BFM_MINE_ATTACHMENT_SLOTS_H
#define BFM_MINE_ATTACHMENT_SLOTS_H

#include "mine_guest_abi.h"
#include "psx_types.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u16 type;
    u8 reserved02[4];
    u16 x;
    u8 reserved08[2];
    u16 y;
    u8 reserved0c[2];
    u16 z;
    u8 reserved10[16];
    u32 object;
    u8 reserved24[20];
} MineAttachedSlot;
#else
typedef struct { u8 bytes[0x38]; } MineAttachedSlot;
#endif

void func_80129010(void);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
/* Each lookup and store describes a single game access. */
int bfm_mine_attachment_handler_at(u32 table_entry, u32 *handler);
int bfm_mine_run_attachment_handler(u32 handler, MineAttachedSlot *slot);
int bfm_mine_store_attachment_half(u32 address, u16 value);
/* Returns zero on the first unresolved access or handler. Earlier effects persist. */
int bfm_mine_update_attachment_slots(void);
#endif

#endif

#ifndef BFM_MINE_FRAME_CALLBACK_H
#define BFM_MINE_FRAME_CALLBACK_H

#include "mine_guest_abi.h"
#include "psx_types.h"

void func_8013BC7C(u8 *record);
void func_8013BCDC(void);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
/* Both values are addresses of game-owned words in this scene member. */
#define BFM_MINE_RECORD_CALLBACK_TABLE 0x8017f0b4u
#define BFM_MINE_PENDING_CALLBACK 0x8017f850u

int bfm_mine_frame_callback_word(u32 address, u32 *value);
int bfm_mine_record_callback(u32 target, u8 *record);
int bfm_mine_pending_callback(u32 target);

int bfm_mine_frame_record_step(u8 *record, unsigned bytes);
int bfm_mine_frame_pending_step(void);
#endif

#endif

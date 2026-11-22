#ifndef BFM_SC02_MINE_ACTOR_LINKS_H
#define BFM_SC02_MINE_ACTOR_LINKS_H

#include "mine_guest_abi.h"
#include "psx_types.h"

void func_80165CA0(void);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
#define BFM_MINE_LINK_SLOTS 0x8011D030u
#define BFM_MINE_LINK_TABLE_POINTER 0x80126728u

/* All accesses use guest addresses so slot and target storage can overlap. */
int bfm_mine_link_read_half(u32 address, u16 *value);
int bfm_mine_link_read_word(u32 address, u32 *value);
int bfm_mine_link_store_half(u32 address, u16 value);
int bfm_mine_link_store_word(u32 address, u32 value);
int bfm_mine_link_run_handler(u32 handler, u32 slot_address);

/* Stops on the first unresolved access. Earlier writes and callbacks are retained. */
int bfm_mine_update_actor_links(void);
#endif

#endif
